# Cellular Signal Measuring Example

## Application definition
The Cellular Signal Measuring application is an example application that embedded several functionalities.

The main purpose of the application is twofold: periodically monitor the signal quality of the cellular environment and report the base stations visible by the cellular module if the `Button #2` is pressed.
All the collected information are stored in the device filesystem via the application log but also shared into the cloud via the SARA-R5 MQTT embedded client. 

Once turned ON, by default the application is monitoring the cellular signal quality. Then, if the `Button #2` is pressed, a base station scan is initialized. 
Differently, if the `Button #1` is pressed the application is turned OFF. 

### Booting functionalities
For debuggability purposes, it is possible to extract the serial log of the past execution if, during the device boot, the `Button #1` is pressed within 5 seconds from power ON.

On the opposite, the log file can be erased if the `Button #2` is pressed within 5 seconds from power ON. 

## Tasks 
This application has 5 tasks:

- LED Task
- Registration Task
- Cell Scan Task
- Signal Quality Task
- MQTT Task

Each task, apart from the LED Task, are based on the same 'boiler plate' design.
Each task has a mixture of a `taskMutex`, `taskEventQueue` and `taskHandler` UBXLIB provided feature.

The `taskMutex` is used to encompass the running of the task or the event of this task. There is a `isXXXXXTaskRunning()` function for each task that the main application can monitor.

The tasks can exit either by simply setting the `gExitApp` variable, or specifically calling the `stopXXX()` type function for that task.

### LED Task
This task monitors the gAppStatus variable and changes the LEDs to show the current state. As this is a running task all three LEDS can be blinked, flashed, turned on/off etc.

The `gAppStatus` is based on an enumerator which inturn has it's own LED configuration for that status.

### Registration Task
This task monitors the registration status and calls the required `NetworkUp()` function if requried. The number of times the networks goes up is counted.

If the network is currently unknown, the other tasks can see this from the `gIsNetworkUp` variable. Generally if the network is not 'up' the other tasks should not send/publish any data, or expect any downlink data.

### Cell Scan Task
This task is run when the Button #2 is pressed. A message is sent to the CellScanEventQueue. The cell scan task performs a cell scan by using the `uCellNetScanGetFirst()` and `uCellNetScanGetNext()` UBXLIB functions.

The results of the network scan are published via MQTT to the defined broker/topic

### Signal Quality Task
This task runs a signal quality query using the `uCellInfoRefreshRadioParameters()` UBXLIB function. The RSRP and RSRQ results are published via MQTT to the defined broker/topic

### MQTT Task
This task waits for a message on it's MQTT event queue. The other tasks use the `sendMQTTMessage()` function to queue their message on the event queue, with the topic and message as parameters.

The event queue has a five message buffer. It will first check if `gIsNetworkUp` variable is set before it goes to publish the message using the `uMqttClientPublish()` UBXLIB function. If the network is not up, the message is not sent.

The MQTT task will also monitor the broker connection, and if it goes down, it will try and re-connect automatically.