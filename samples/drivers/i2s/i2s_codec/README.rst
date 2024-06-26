.. zephyr:code-sample:: i2s_codec
   :name: I2S codec
   :relevant-api: i2s_interface

   Process an audio stream to codec.

Overview
********

This sample demonstrates how to use an I2S driver in a simple processing of
an audio stream. It configures and starts from memory buffer or from DMIC to
record i2s data and send to codec(on board WM8904) with DMA.

Requirements
************

This sample has been tested on mimxrt685_evk_cm33

Building and Running
********************

The code can be found in :zephyr_file:`samples/drivers/i2s/i2s_codec`.

To build and flash the application:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/i2s/i2s_codec
   :board: mimxrt685_evk_cm33
   :goals: build flash
   :compact:

To run you can connect earPhone from the lineout connect and here the sound
from DMIC or from memory buffer.