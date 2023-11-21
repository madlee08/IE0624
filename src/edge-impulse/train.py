import sys, os, shutil, signal, random, operator, functools, time, subprocess, math, contextlib, io, skimage, argparse
import logging, threading

dir_path = os.path.dirname(os.path.realpath(__file__))

parser = argparse.ArgumentParser(description='Edge Impulse training scripts')
parser.add_argument('--info-file', type=str, required=False,
                    help='train_input.json file with info about classes and input shape',
                    default=os.path.join(dir_path, 'train_input.json'))
parser.add_argument('--data-directory', type=str, required=True,
                    help='Where to read the data from')
parser.add_argument('--out-directory', type=str, required=True,
                    help='Where to write the data')

parser.add_argument('--epochs', type=int, required=False,
                    help='Number of training cycles')
parser.add_argument('--learning-rate', type=float, required=False,
                    help='Learning rate')
parser.add_argument('--batch_size', type=int, required=False,
                    help='Training batch size')
parser.add_argument('--ensure-determinism', action='store_true',
                    help='Prevent non-determinism, e.g. do not shuffle batches')

args, unknown = parser.parse_known_args()

# Info about the training pipeline (inputs / shapes / modes etc.)
if not os.path.exists(args.info_file):
    print('Info file', args.info_file, 'does not exist')
    exit(1)

logging.getLogger('tensorflow').setLevel(logging.ERROR)
logging.disable(logging.WARNING)
os.environ["KMP_AFFINITY"] = "noverbose"
os.environ['TF_CPP_MIN_LOG_LEVEL'] = '3'

import tensorflow as tf

import numpy as np

# Suppress Numpy deprecation warnings
# TODO: Only suppress warnings in production, not during development
import warnings
warnings.filterwarnings('ignore', category=np.VisibleDeprecationWarning)
# Filter out this erroneous warning (https://stackoverflow.com/a/70268806 for context)
warnings.filterwarnings('ignore', 'Custom mask layers require a config and must override get_config')

RANDOM_SEED = 3
random.seed(RANDOM_SEED)
np.random.seed(RANDOM_SEED)
tf.random.set_seed(RANDOM_SEED)
tf.keras.utils.set_random_seed(RANDOM_SEED)

tf.get_logger().setLevel('ERROR')
tf.autograph.set_verbosity(3)

# Since it also includes TensorFlow and numpy, this library should be imported after TensorFlow has been configured
sys.path.append('./resources/libraries')
import ei_tensorflow.training
import ei_tensorflow.conversion
import ei_tensorflow.profiling
import ei_tensorflow.inference
import ei_tensorflow.embeddings
import ei_tensorflow.lr_finder
import ei_tensorflow.brainchip.model
import ei_tensorflow.gpu
from ei_shared.parse_train_input import parse_train_input, parse_input_shape
from ei_augmentation.specaugment import SpecAugment

import json, datetime, time, traceback
from sklearn.model_selection import train_test_split
from sklearn.metrics import log_loss, mean_squared_error

input = parse_train_input(args.info_file)

# Small hack to detect on profile stage if Custom Learning Block produced Akida model
# On creating profile script, Studio is not aware that CLB can produce Akida model
if os.path.exists(os.path.join(args.out_directory, 'akida_model.fbz')):
    input.akidaModel = True

# For SSD object detection models we specify batch size via 'input'.
# We must allow it to be overridden via command line args:
if input.mode == 'object-detection' and 'batch_size' in args and args.batch_size is not None:
    input.objectDetectionBatchSize = args.batch_size

BEST_MODEL_PATH = os.path.join(os.sep, 'tmp', 'best_model.tf' if input.akidaModel else 'best_model.hdf5')

# Information about the data and input:
# The shape of the model's input (which may be different from the shape of the data)
MODEL_INPUT_SHAPE = parse_input_shape(input.inputShapeString)
# The length of the model's input, used to determine the reshape inside the model
MODEL_INPUT_LENGTH = MODEL_INPUT_SHAPE[0]
MAX_TRAINING_TIME_S = input.maxTrainingTimeSeconds
MAX_GPU_TIME_S = input.remainingGpuComputeTimeSeconds

online_dsp_config = None

if (online_dsp_config != None):
    print('The online DSP experiment is enabled; training will be slower than normal.')

# load imports dependening on import
if (input.mode == 'object-detection' and input.objectDetectionLastLayer == 'mobilenet-ssd'):
    import ei_tensorflow.object_detection

def exit_gracefully(signum, frame):
    print("")
    print("Terminated by user", flush=True)
    time.sleep(0.2)
    sys.exit(1)


def train_model(train_dataset, validation_dataset, input_length, callbacks,
                X_train, X_test, Y_train, Y_test, train_sample_count, classes, classes_values,
                ensure_determinism=False):
    global ei_tensorflow

    override_mode = None
    disable_per_channel_quantization = False
    # We can optionally output a Brainchip Akida pre-trained model
    akida_model = None
    akida_edge_model = None

    if (input.mode == 'object-detection' and input.objectDetectionLastLayer == 'mobilenet-ssd'):
        ei_tensorflow.object_detection.set_limits(max_training_time_s=MAX_TRAINING_TIME_S,
            max_gpu_time_s=MAX_GPU_TIME_S,
            is_enterprise_project=input.isEnterpriseProject)

    import tensorflow as tf
    from tensorflow.keras.models import Sequential
    from tensorflow.keras.layers import Dense, InputLayer, Dropout, Conv1D, Conv2D, Flatten, Reshape, MaxPooling1D, MaxPooling2D, AveragePooling2D, BatchNormalization, Permute, ReLU, Softmax
    from tensorflow.keras.optimizers.legacy import Adam
    
    # Data augmentation for spectrograms, which can be configured in visual mode.
    # To learn what these arguments mean, see the SpecAugment paper:
    # https://arxiv.org/abs/1904.08779
    sa = SpecAugment(spectrogram_shape=[int(input_length / 13), 13], mF_num_freq_masks=0, F_freq_mask_max_consecutive=0, mT_num_time_masks=1, T_time_mask_max_consecutive=1, enable_time_warp=False, W_time_warp_max_distance=6, mask_with_mean=False)
    train_dataset = train_dataset.map(sa.mapper(), num_parallel_calls=tf.data.AUTOTUNE)
    
    EPOCHS = args.epochs or 100
    LEARNING_RATE = args.learning_rate or 0.005
    # If True, non-deterministic functions (e.g. shuffling batches) are not used.
    # This is False by default.
    ENSURE_DETERMINISM = args.ensure_determinism
    # this controls the batch size, or you can manipulate the tf.data.Dataset objects yourself
    BATCH_SIZE = args.batch_size or 32
    if not ENSURE_DETERMINISM:
        train_dataset = train_dataset.shuffle(buffer_size=BATCH_SIZE*4)
    train_dataset=train_dataset.batch(BATCH_SIZE, drop_remainder=False)
    validation_dataset = validation_dataset.batch(BATCH_SIZE, drop_remainder=False)
    
    # model architecture
    model = Sequential()
    # Data augmentation, which can be configured in visual mode
    model.add(tf.keras.layers.GaussianNoise(stddev=0.2))
    channels = 1
    columns = 13
    rows = int(input_length / (columns * channels))
    model.add(Reshape((rows, columns, channels), input_shape=(input_length, )))
    model.add(Conv2D(8, kernel_size=3, kernel_constraint=tf.keras.constraints.MaxNorm(1), padding='same', activation='relu'))
    model.add(MaxPooling2D(pool_size=2, strides=2, padding='same'))
    model.add(Dropout(0.5))
    model.add(Conv2D(16, kernel_size=3, kernel_constraint=tf.keras.constraints.MaxNorm(1), padding='same', activation='relu'))
    model.add(MaxPooling2D(pool_size=2, strides=2, padding='same'))
    model.add(Dropout(0.5))
    model.add(Flatten())
    model.add(Dense(classes, name='y_pred', activation='softmax'))
    
    # this controls the learning rate
    opt = Adam(learning_rate=LEARNING_RATE, beta_1=0.9, beta_2=0.999)
    callbacks.append(BatchLoggerCallback(BATCH_SIZE, train_sample_count, epochs=EPOCHS, ensure_determinism=ENSURE_DETERMINISM))
    
    # train the neural network
    model.compile(loss='categorical_crossentropy', optimizer=opt, metrics=['accuracy'])
    model.fit(train_dataset, epochs=EPOCHS, validation_data=validation_dataset, verbose=2, callbacks=callbacks)
    
    # Use this flag to disable per-channel quantization for a model.
    # This can reduce RAM usage for convolutional models, but may have
    # an impact on accuracy.
    disable_per_channel_quantization = False
    return model, override_mode, disable_per_channel_quantization, akida_model, akida_edge_model

# This callback ensures the frontend doesn't time out by sending a progress update every interval_s seconds.
# This is necessary for long running epochs (in big datasets/complex models)
class BatchLoggerCallback(tf.keras.callbacks.Callback):
    def __init__(self, batch_size, train_sample_count, epochs, interval_s = 10, ensure_determinism=False):
        # train_sample_count could be smaller than the batch size, so make sure total_batches is atleast
        # 1 to avoid a 'divide by zero' exception in the 'on_train_batch_end' callback.
        self.total_batches = max(1, int(train_sample_count / batch_size))
        self.last_log_time = time.time()
        self.epochs = epochs
        self.interval_s = interval_s
        print(f'Using batch size: {batch_size}', flush=True)

    # Within each epoch, print the time every 10 seconds
    def on_train_batch_end(self, batch, logs=None):
        current_time = time.time()
        if self.last_log_time + self.interval_s < current_time:
            print('Epoch {0}% done'.format(int(100 / self.total_batches * batch)), flush=True)
            self.last_log_time = current_time

    # Reset the time the start of every epoch
    def on_epoch_end(self, epoch, logs=None):
        self.last_log_time = time.time()

def main_function():
    """This function is used to avoid contaminating the global scope"""
    classes_values = input.classes
    classes = 1 if input.mode == 'regression' else len(classes_values)

    mode = input.mode
    object_detection_last_layer = input.objectDetectionLastLayer if input.mode == 'object-detection' else None

    train_dataset, validation_dataset, samples_dataset, X_train, X_test, Y_train, Y_test, has_samples, X_samples, Y_samples = ei_tensorflow.training.get_dataset_from_folder(
        input, args.data_directory, RANDOM_SEED, online_dsp_config, MODEL_INPUT_SHAPE, args.ensure_determinism
    )

    callbacks = ei_tensorflow.training.get_callbacks(dir_path, mode, BEST_MODEL_PATH,
        object_detection_last_layer=object_detection_last_layer,
        is_enterprise_project=input.isEnterpriseProject,
        max_training_time_s=MAX_TRAINING_TIME_S,
        max_gpu_time_s=MAX_GPU_TIME_S,
        enable_tensorboard=input.tensorboardLogging)

    model = None

    print('')
    print('Training model...')
    ei_tensorflow.gpu.print_gpu_info()
    print('Training on {0} inputs, validating on {1} inputs'.format(len(X_train), len(X_test)))
    # USER SPECIFIC STUFF
    model, override_mode, disable_per_channel_quantization, akida_model, akida_edge_model = train_model(train_dataset, validation_dataset,
        MODEL_INPUT_LENGTH, callbacks, X_train, X_test, Y_train, Y_test, len(X_train), classes, classes_values, args.ensure_determinism)
    if override_mode is not None:
        mode = override_mode
    # END OF USER SPECIFIC STUFF

    # REST OF THE APP
    print('Finished training', flush=True)
    print('', flush=True)

    # Make sure these variables are here, even when quantization fails
    tflite_quant_model = None

    if mode == 'object-detection':
        tflite_model, tflite_quant_model = ei_tensorflow.object_detection.convert_to_tf_lite(
            args.out_directory,
            saved_model_dir='saved_model',
            validation_dataset=validation_dataset,
            model_filenames_float='model.tflite',
            model_filenames_quantised_int8='model_quantized_int8_io.tflite')
    elif mode == 'segmentation':
        from ei_tensorflow.constrained_object_detection.conversion import convert_to_tf_lite
        tflite_model, tflite_quant_model = convert_to_tf_lite(
            args.out_directory, model,
            saved_model_dir='saved_model',
            h5_model_path='model.h5',
            validation_dataset=validation_dataset,
            model_filenames_float='model.tflite',
            model_filenames_quantised_int8='model_quantized_int8_io.tflite',
            disable_per_channel=disable_per_channel_quantization)
        if input.akidaModel:
            if not akida_model:
                print('Akida training code must assign a quantized model to a variable named "akida_model"', flush=True)
                exit(1)
            ei_tensorflow.brainchip.model.convert_akida_model(args.out_directory, akida_model,
                                                            'akida_model.fbz',
                                                            MODEL_INPUT_SHAPE)
    else:
        model, tflite_model, tflite_quant_model = ei_tensorflow.conversion.convert_to_tf_lite(
            model, BEST_MODEL_PATH, args.out_directory,
            saved_model_dir='saved_model',
            h5_model_path='model.h5',
            validation_dataset=validation_dataset,
            model_input_shape=MODEL_INPUT_SHAPE,
            model_filenames_float='model.tflite',
            model_filenames_quantised_int8='model_quantized_int8_io.tflite',
            disable_per_channel=disable_per_channel_quantization,
            syntiant_target=input.syntiantTarget,
            akida_model=input.akidaModel)

        if input.akidaModel:
            if not akida_model:
                print('Akida training code must assign a quantized model to a variable named "akida_model"', flush=True)
                exit(1)

            ei_tensorflow.brainchip.model.convert_akida_model(args.out_directory, akida_model,
                                                              'akida_model.fbz',
                                                              MODEL_INPUT_SHAPE)
            if input.akidaEdgeModel:
                ei_tensorflow.brainchip.model.convert_akida_model(args.out_directory, akida_edge_model,
                                                                'akida_edge_learning_model.fbz',
                                                                MODEL_INPUT_SHAPE)
            else:
                import os
                model_full_path = os.path.join(args.out_directory, 'akida_edge_learning_model.fbz')
                if os.path.isfile(model_full_path):
                    os.remove(model_full_path)

if __name__ == "__main__":
    signal.signal(signal.SIGINT, exit_gracefully)
    signal.signal(signal.SIGTERM, exit_gracefully)

    main_function()