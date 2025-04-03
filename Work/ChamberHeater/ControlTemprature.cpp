/**
 * @Author: lojainadly
 * @Date:   2025-01-20
 * @Last modified by: lojainadly
 * @Last modified time: 2025-04-03
 */

#include "ControlTemperature.h"

extern SPI_HandleTypeDef hspi2;

ControlTemperature::ControlTemperature():
max31865(),
m_ThreadID(0),
m_taskStatus(pdFAIL),
m_currentTemp(0.0),
m_setTemp(0.0),
m_tempOffset(0.0),
m_start(false),
m_heat(false),
m_tempOn(true),
m_tempSet(false),
read(false),
temp(0.0)
{
}

ControlTemperature::~ControlTemperature()
{
}

// **************************
// ***** Public methods *****
// **************************

 uint8_t ControlTemperature::StartTasks()
 {
     if(ControlBase::StartTasks() == 0)
     {
             m_controlTempTask_attributes.name = "controlHeaterTask";
             m_controlTempTask_attributes.stack_size = 1024;
             m_controlTempTask_attributes.priority = (osPriority_t) osPriorityNormal;
             m_taskTemp = osThreadNew(StartTaskControlTemperature, this, &m_controlTempTask_attributes);
             if(m_taskTemp != NULL)
             {
                 m_controlHeaterTask_attributes.name = "controlTempTask";
                 m_controlHeaterTask_attributes.stack_size = 1024;
                 m_controlHeaterTask_attributes.priority = (osPriority_t) osPriorityNormal;
                 m_taskHeater = osThreadNew(StartTaskControlHeater, this, &m_controlHeaterTask_attributes);
                 if(m_taskHeater != NULL)
                 {
                     return 0;
                 }
              }
     }
     return 2;
 }

 float ControlTemperature::GetTemp()
 {
    UpdateTemp();
    return m_currentTemp.load();
 }

 float ControlTemperature::GetGoalTemp()
 {
     return m_setTemp.load();
 }

// *****************************
// ***** Protected methods *****
// *****************************

 bool ControlTemperature::ProcessCommands(ControlCommandData& commandData)
 {
     if(ControlBase::ProcessCommands(commandData) == false)
     {
         if(commandData.paramSize() == 1)
         {
             if(Help(commandData))
             {
                 return true;
             }
         }
         else if(commandData.paramValid(0))
         {
             if(Temp(commandData))
             {
                 return true;
             }
             if(Heat(commandData))
             {
                 return true;
             }
         }
     }
     return false;
 }

 bool ControlTemperature::Help(ControlCommandData& commandData)
 {
     if(commandData.isCommand(MCU_CMD_HELP))
     {
         dataSendSerial(HELP_CMD_FORMAT_STR ":\n", MCU_CMD_TEMP);
         dataSendSerial(HELP_CMD_FORMAT_SUB_STR " - display current temperature\n", MCU_CMD_TEMP_DISPLAY);
         dataSendSerial(HELP_CMD_FORMAT_SUB_STR " - sets the goal temperature\n", MCU_CMD_TEMP_SET);
         dataSendSerial(HELP_CMD_FORMAT_SUB_STR " - resets the goal temperature\n", MCU_CMD_TEMP_RESET);
         dataSendSerial(HELP_CMD_FORMAT_STR ":\n", MCU_CMD_HEAT);
         dataSendSerial(HELP_CMD_FORMAT_SUB_STR " - turns heater on\n", MCU_CMD_HEAT_ON );
         dataSendSerial(HELP_CMD_FORMAT_SUB_STR " - turns heater off\n", MCU_CMD_HEAT_OFF);
         return true;
     }
     return false;
 }

bool ControlTemperature::Temp(ControlCommandData& commandData)
{
    if(0 == strcmpi(commandData.getParam(0), MCU_CMD_TEMP))
    {
        if(commandData.paramValid(1))
        {
            if(0 == strcmpi(commandData.getParam(1), MCU_CMD_TEMP_DISPLAY))
            {
                UpdateTemp();
                dataSendSerial(TEMP_CMD_FORMAT_FLOAT, m_currentTemp.load());
                return true;
            }
            else if(0 == strcmpi(commandData.getParam(1), MCU_CMD_TEMP_SET))
            {
                m_setTemp.store(atof(commandData.getParam(2)));
                return true;
            }
            else if((commandData.paramValid(2)) && ((0 == strcmpi(commandData.getParam(1), MCU_CMD_TEMP_RESET))))
            {
                m_setTemp.store(0.00);
                return true;
            }
        }
    }
    return false;
}

bool ControlTemperature::Heat(ControlCommandData& commandData)
{
    if(0 == strcmpi(commandData.getParam(0), MCU_CMD_HEAT))
    {
        if(commandData.paramValid(1))
        {
            if(0 == strcmpi(commandData.getParam(1), MCU_CMD_HEAT_ON))
            {
                HeatOn();
                return true;
            }
            if(0 == strcmpi(commandData.getParam(1), MCU_CMD_HEAT_OFF))
            {
                HeatOff();
                return true;
            }
        }
    }
    return false;
}

 // ***************************
 // ***** Private methods *****
 // ***************************

 void ControlTemperature::StartTaskControlTemperature(void *arg)
 {
    ControlTemperature *ptrControlTemperature = static_cast<ControlTemperature *>(arg);
    if(ptrControlTemperature != NULL)
    {
        ptrControlTemperature -> TaskControlTemperature();
    }

    while(true)
    {
    }
 }

 void ControlTemperature::StartTaskControlHeater(void *arg)
 {
     ControlTemperature *ptrControlTemperature = static_cast<ControlTemperature *>(arg);
     if(ptrControlTemperature != NULL)
     {
         ptrControlTemperature -> TaskControlHeater();
     }
     while(true)
     {
     }
 }

void ControlTemperature::TaskControlTemperature()
{
    Max31865_init(&max31865, &hspi2, GPIOA, GPIO_PIN_8, 3, 50);
    TemperatureStatus();
}

void ControlTemperature::TaskControlHeater()
{
    TouchSensor();
}

void ControlTemperature::UpdateTemp()
{
    read = Max31865_readTempC(&max31865, &temp);
    if(read){
        m_currentTemp.store(temp);
    }
}

void ControlTemperature::HeatOn()
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
    m_heat = true;
    dataSendSerial("Turned heat on\n");
}

void ControlTemperature::HeatOff()
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
    m_heat = false;
    dataSendSerial("Turned heat off\n");
}

void ControlTemperature::TouchSensor()
{
    GPIO_PinState sensorState = GPIO_PIN_RESET;
    GPIO_PinState prevSensorState = GPIO_PIN_RESET;

    while(true)
    {
        sensorState = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_11);
        if(sensorState != prevSensorState)
        {
            if((sensorState == GPIO_PIN_SET) && (prevSensorState == GPIO_PIN_RESET))
            {
                if(m_heat == false)
                {
                    HeatOn();
                    if(m_tempSet)
                    {
                        m_tempOn = true;
                    }
                }
                else if(m_heat == true)
                {
                    HeatOff();
                    m_tempOn = false;
                }
                else
                {
                    osDelay(500);
                }
            }
            prevSensorState = sensorState;
        }
    }
}

void ControlTemperature::TemperatureStatus()
{
      // constants for flat heating pad
    int kp = 1.2;
    int ki = 0.7;
    int kd = 2.4;

    float actual_temp;
    float past_temp = 0.0;
    float error;
    float integral;
    float derivative;
    float pwm;
    float prev_error;
    float buffer_on = 3.0;
    float buffer_off = 0.2;

    osDelay(2000);
    dataSendSerial("starting PID loop\n");

    while(true)
    {
        if (m_tempOn)
        {
            GetTemp();
            actual_temp = m_currentTemp.load();
            error = (m_setTemp.load()) - actual_temp;
            integral = integral + error;
            derivative = error - prev_error;
            pwm = (kp * error) + (ki * integral) + (kd * derivative);

            dataSendSerial("Curr Temp: %.2f, pwm: %.2f\n", actual_temp, pwm);

            if ((actual_temp >= (m_setTemp.load() - buffer_on)) && (past_temp <= actual_temp))
            {
                if (m_heat && pwm <= 3)
                {
                    HeatOff();
                }
            }

            if ((actual_temp <= (m_setTemp.load() - buffer_off)) && (past_temp >= actual_temp))
            {
                if (!m_heat && pwm > 0.01)
                {
                    HeatOn();

                }
            }

//            else
//            {
//                if ((pwm > 0.1) && (!m_heat))
//                {
//                    HeatOn();
//                }
//                else if ((pwm < 0.1) && (m_heat))
//                {
//                    HeatOff();
//                }
//            }

            prev_error = error;
            osDelay(500);

            past_temp = actual_temp;

        }
        else
        {
            osDelay(100);
        }
    }
}
