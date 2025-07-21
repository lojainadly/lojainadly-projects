/**
 * @Author: Lojain Adly
 * @Date: 2023-09-29
 * @Last Modified: 2025-07-02
*/

#include "ControlScopeLight.h"

#define SCOPE_LIGHTS_OFF        0
#define CHAMBER_LIGHTS_OFF      0
#define SCOPE_LIGHTS_ON         32
#define SCOPE_LIGHTS_BOUNDRY    255
#define CHAMBER_LIGHTS_BOUNDRY  4095
#define CHAMBER_RED_INCREMENT   419
#define CHAMBER_GREEN_INCREMENT 319
#define CHAMBER_BLUE_INCREMENT  219
#define CHAMBER_RED_THRESHOLD   2000
#define CHAMBER_GREEN_THRESHOLD 2500
#define CHAMBER_BLUE_THRESHOLD  3000
#define ARRAY_SIZE              10
#define GPIO_PIN_TOUCH_SENSOR   GPIO_PIN_10
#define R_10                    2200
#define R_20                    2400
#define R_30                    2600
#define R_40                    2800
#define R_50                    2950
#define R_60                    3254
#define R_70                    3463
#define R_80                    3672
#define R_90                    3881
#define R_100                   4095
#define G_10                    2700
#define G_20                    2900
#define G_30                    3100
#define G_40                    3300
#define G_50                    3200
#define G_60                    3450
#define G_70                    3520
#define G_80                    3680
#define G_90                    3840
#define G_100                   4095
#define B_10                    3000
#define B_20                    3100
#define B_30                    3200
#define B_40                    3300
#define B_50                    3400
#define B_60                    3660
#define B_70                    3770
#define B_80                    3880
#define B_90                    3990
#define B_100                   4095


extern TIM_HandleTypeDef htim2;
extern DAC_HandleTypeDef hdac1;
extern DAC_HandleTypeDef hdac2;

ControlScopeLight::ControlScopeLight() :  m_start(false),
                                          m_taskStatus(pdFAIL),
                                          m_ledStateCurr(false),
                                          m_ledStatePrev(false),
                                          m_ledState(false),
                                          m_led(false),
                                          m_chamberLed(false),
                                          t_chamberSensorCurr(false),
                                          t_chamberSensorPrev(false),
                                          t_sensorCurr(false),
                                          t_sensorPrev(false),
                                          m_scopeRed(SCOPE_LIGHTS_OFF),
                                          m_scopeGreen(SCOPE_LIGHTS_ON),
                                          m_scopeBlue(SCOPE_LIGHTS_OFF),
                                          m_chamberRed(CHAMBER_LIGHTS_OFF),
                                          m_chamberGreen(3000),
                                          m_chamberBlue(CHAMBER_LIGHTS_OFF),
                                          m_chamberWhite(CHAMBER_LIGHTS_OFF),
                                          m_touchSensorEnabled(true)
{
    memset(m_pixel, 0x00, sizeof(m_pixel));
    memset(m_dmaBuffer, 0x00, sizeof(m_dmaBuffer));
}

ControlScopeLight::~ControlScopeLight()
{
    HAL_DAC_Stop(&hdac1, DAC_CHANNEL_1);
    HAL_DAC_Stop(&hdac1, DAC_CHANNEL_2);
    HAL_DAC_Stop(&hdac2, DAC_CHANNEL_1);
}

// **************************
// ***** Public Methods *****
// **************************

uint8_t ControlScopeLight::StartTasks()
{

    if (false == m_start)
    {
        if(0 == ControlBase::StartTasks())
        {
            m_scopeLightsDataTask_attributes.name = "serialStartTask";
            m_scopeLightsDataTask_attributes.stack_size = 2048;
            m_scopeLightsDataTask_attributes.priority = (osPriority_t) osPriorityNormal1;

            m_chamberLightsDataTask_attributes.name = "serialChamberStartTask";
            m_chamberLightsDataTask_attributes.stack_size = 2048;
            m_chamberLightsDataTask_attributes.priority = (osPriority_t) osPriorityNormal1;

            m_scopeLightsDataTaskHandle = osThreadNew(StartTaskControlScopeLight, this, &m_scopeLightsDataTask_attributes);
            m_chamberLightsDataTaskHandle = osThreadNew(StartTaskControlChamberLight, this, &m_chamberLightsDataTask_attributes);
        }

        if ((NULL != m_scopeLightsDataTaskHandle) && (NULL != m_chamberLightsDataTaskHandle))
        {

           return 0;
        }
    }

    return 4;
}


// *****************************
// ***** Protected Methods *****
// *****************************

bool ControlScopeLight::Help(ControlCommandData& commandData)
{
    ControlBase::Help(commandData);
    if (commandData.isCommand(MCU_CMD_HELP))
    {
        dataSendSerial(HELP_CMD_FORMAT_STR " - set scope lights settings\n", MCU_CMD_SCL);
        dataSendSerial(HELP_CMD_FORMAT_SUB_STR " - set scope lights on\n", MCU_CMD_SCL_ON);
        dataSendSerial(HELP_CMD_FORMAT_SUB_STR " - set scope lights off\n", MCU_CMD_SCL_OFF);
        dataSendSerial(HELP_CMD_FORMAT_SUB_STR " - enable touch sensor for scope lights\n", MCU_CMD_SCL_ENABLE);
        dataSendSerial(HELP_CMD_FORMAT_SUB_STR " - disable touch sensor for scope lights\n", MCU_CMD_SCL_DISABLE);
        dataSendSerial(HELP_CMD_FORMAT_SUB_STR " - set scope lights red, green, and blue values\n", "rrr ggg bbb");
        dataSendSerial(HELP_CMD_FORMAT_SUB_STR " - set scope lights green\n", MCU_CMD_SCL_GREEN);
        dataSendSerial(HELP_CMD_FORMAT_SUB_STR " - set scope lights red\n", MCU_CMD_SCL_RED);
        dataSendSerial(HELP_CMD_FORMAT_SUB_STR " - set scope lights blue\n", MCU_CMD_SCL_BLUE);
        dataSendSerial(HELP_CMD_FORMAT_SUB_STR " - set scope lights white\n", MCU_CMD_SCL_WHITE);
        dataSendSerial(HELP_CMD_FORMAT_SUB_STR " - set scope lights yellow\n", MCU_CMD_SCL_YELLOW);

        dataSendSerial(HELP_CMD_FORMAT_STR " - set chamber lights settings\n", MCU_CMD_CML);
        dataSendSerial(HELP_CMD_FORMAT_SUB_STR "DOUBLE TAP to increase brightness!\n");
        dataSendSerial(HELP_CMD_FORMAT_SUB_STR " - set chamber lights on\n", MCU_CMD_CML_ON);
        dataSendSerial(HELP_CMD_FORMAT_SUB_STR " - set chamber lights off\n", MCU_CMD_CML_OFF);
        dataSendSerial(HELP_CMD_FORMAT_SUB_STR " - set chamber lights red, green, and blue values\n", "rrr ggg bbb");
        dataSendSerial(HELP_CMD_FORMAT_SUB_STR " - set chamber lights green\n", MCU_CMD_CML_GREEN);
        dataSendSerial(HELP_CMD_FORMAT_SUB_STR " - set chamber lights red\n", MCU_CMD_CML_RED);
        dataSendSerial(HELP_CMD_FORMAT_SUB_STR " - set chamber lights blue\n", MCU_CMD_CML_BLUE);
        dataSendSerial(HELP_CMD_FORMAT_SUB_STR " - set chamber lights white\n", MCU_CMD_CML_WHITE);

        return true;
    }
    return false;
}

bool ControlScopeLight::IFF(ControlCommandData& commandData)
 {
     if (commandData.isCommand(MCU_CMD_IFF))
     {
         dataSendSerial("%s\n", MCU_ID_109_ScopeLights);
         dataSendFDCAN("%s\n", MCU_ID_109_ScopeLights);
         return true;
     }

     return false;
 }

bool ControlScopeLight::ProcessCommands(ControlCommandData& commandData)
{
    if (false == ControlBase::ProcessCommands(commandData))
    {
        if (commandData.isCommand(MCU_CMD_SCL_ON))
        {
            ScopeLightsOn();
            m_led = true;
            return true;
        }

        if (commandData.isCommand(MCU_CMD_SCL_OFF))
        {
            ScopeLightsOff();
            m_led = false;
            return true;
        }

        if (commandData.isCommand(MCU_CMD_SCL_RED))
        {
            m_scopeRed = SCOPE_LIGHTS_ON;
            m_scopeGreen = SCOPE_LIGHTS_OFF;
            m_scopeBlue = SCOPE_LIGHTS_OFF;
            m_led = true;
            ScopeLightsOn();
            return true;
        }

        if (commandData.isCommand(MCU_CMD_SCL_GREEN))
        {
            m_scopeRed = SCOPE_LIGHTS_OFF;
            m_scopeGreen = SCOPE_LIGHTS_ON;
            m_scopeBlue = SCOPE_LIGHTS_OFF;
            m_led = true;
            ScopeLightsOn();
            return true;
        }

        if (commandData.isCommand(MCU_CMD_SCL_BLUE))
        {
            m_scopeRed = SCOPE_LIGHTS_OFF;
            m_scopeGreen = SCOPE_LIGHTS_OFF;
            m_scopeBlue = SCOPE_LIGHTS_ON;
            m_led = true;
            ScopeLightsOn();
            return true;
        }

        if (commandData.isCommand(MCU_CMD_SCL_WHITE))
        {
            m_scopeRed = 150;
            m_scopeGreen = 80;
            m_scopeBlue = 100;
            m_led = true;
            ScopeLightsOn();
            return true;
        }

        if (commandData.isCommand(MCU_CMD_SCL_YELLOW))
        {
            m_scopeRed = 250;
            m_scopeGreen = 140;
            m_scopeBlue = 15;
            m_led = true;
            ScopeLightsOn();
            return true;
        }
        if (commandData.isCommand(MCU_CMD_SCL_ENABLE))
        {
            EnableTouchSensor();
            return true;
        }

        if (commandData.isCommand(MCU_CMD_SCL_DISABLE))
        {
            DisableTouchSensor();
            return true;
        }
        if (commandData.isCommand(MCU_CMD_SCL) && 4 == commandData.paramSize())
        {
            if (ScopeLightsSet(commandData))
                return true;
        }
        if (commandData.isCommand(MCU_CMD_CML_ON))
        {
            ChamberLightsOn(m_chamberRed, m_chamberGreen, m_chamberBlue);
            return true;
        }

        if (commandData.isCommand(MCU_CMD_CML_RED))
        {
            int red;
            if (commandData.paramToInt(2, red))
            {
                m_chamberRed = red;
                ChamberLightsBoundries(m_chamberRed);
                m_chamberGreen = CHAMBER_LIGHTS_OFF;
                m_chamberBlue = CHAMBER_LIGHTS_OFF;
                ChamberLightsOn(m_chamberRed, m_chamberGreen, m_chamberBlue);
            }
            return true;
        }

        if (commandData.isCommand(MCU_CMD_CML_GREEN))
        {
            int green;
            if (commandData.paramToInt(2, green))
            {
                m_chamberGreen = green;
                ChamberLightsBoundries(m_chamberGreen);
                m_chamberRed = CHAMBER_LIGHTS_OFF;
                m_chamberBlue = CHAMBER_LIGHTS_OFF;
                ChamberLightsOn(m_chamberRed, m_chamberGreen, m_chamberBlue);
            }
            return true;
        }

        if (commandData.isCommand(MCU_CMD_CML_BLUE))
        {
            int blue;
            if (commandData.paramToInt(2, blue))
            {
                m_chamberBlue = blue;
                ChamberLightsBoundries(m_chamberBlue);
                m_chamberRed = CHAMBER_LIGHTS_OFF;
                m_chamberGreen = CHAMBER_LIGHTS_OFF;
                ChamberLightsOn(m_chamberRed, m_chamberGreen, m_chamberBlue);
            }
            return true;
        }
        if (commandData.isCommand(MCU_CMD_CML_WHITE))
        {
            float percent = std::atof(commandData.getParam(2));
            WhiteSettings(percent);
            return true;
        }

        if (commandData.isCommand(MCU_CMD_CML_OFF))
        {
            ChamberLightsOff();
            return true;
        }
        if (commandData.isCommand(MCU_CMD_CML) && 4 == commandData.paramSize())
        {
            if (ChamberLightsSet(commandData))
                return true;
        }
    }
    return false;
}

void ControlScopeLight::StartTaskControlScopeLight(void *arg)
{
    ControlScopeLight *ptrControlScopeLight = static_cast<ControlScopeLight *>(arg);
    if (NULL != ptrControlScopeLight)
        ptrControlScopeLight->TaskControlScopeLight();

    while (1)
    {
    }
}

void ControlScopeLight::StartTaskControlChamberLight(void *arg)
{
    ControlScopeLight *ptrControlChamberLight = static_cast<ControlScopeLight *>(arg);
    if (NULL != ptrControlChamberLight)
    ptrControlChamberLight->TaskControlChamberLight();

    while (1)
    {
    }
}

bool ControlScopeLight::ScopeLightsSet(ControlCommandData& commandData)
{
    if (commandData.paramValid(1) && commandData.paramValid(2) && commandData.paramValid(3))
       {
           int red;
           if (commandData.paramToInt(1, red))
           {
               int green;
               if (commandData.paramToInt(2, green))
               {
                   int blue;
                   if (commandData.paramToInt(3, blue))
                   {
                       ScopeLightsBoundries(m_scopeRed);
                       m_scopeRed = (uint8_t)red;

                       ScopeLightsBoundries(m_scopeGreen);
                       m_scopeGreen = (uint8_t)green;

                       ScopeLightsBoundries(m_scopeBlue);
                       m_scopeBlue = (uint8_t)blue;

                       ScopeLightsOn();
                       return true;
                   }
               }
           }
       }
       return false;
}

bool ControlScopeLight::ChamberLightsSet(ControlCommandData& commandData)
{
    if (commandData.paramValid(1) && commandData.paramValid(2) && commandData.paramValid(3))
    {
        int red;
        if (commandData.paramToInt(1, red))
        {
            int green;
            if (commandData.paramToInt(2, green))
            {
                int blue;
                if (commandData.paramToInt(3, blue))
                {
                    m_chamberRed = red;
                    ChamberLightsBoundries(m_chamberRed);

                    m_chamberGreen = green;
                    ChamberLightsBoundries(m_chamberGreen);

                    m_chamberBlue = blue;
                    ChamberLightsBoundries(m_chamberBlue);

                    ChamberLightsOn(m_chamberRed, m_chamberGreen, m_chamberBlue);
                    return true;
                }
            }
        }
    }
    return false;
}

void ControlScopeLight::NeoPixelShow()
{
    m_pBuff = m_dmaBuffer; //arrays

    for (m_indexPixel = 0; m_indexPixel < NUM_PIXELS; ++m_indexPixel)
    {
        for (m_indexPixelBit = (PIXEL_BIT_SIZE - 1); 0 <= m_indexPixelBit; --m_indexPixelBit)
        {
            if ((m_pixel[m_indexPixel].data >> m_indexPixelBit) & 0x01)
                *m_pBuff = NEO_PIXEL_1;
            else
                *m_pBuff = NEO_PIXEL_0;

            ++m_pBuff;
        }
    }

    // Last element must be 0
    m_dmaBuffer[DMA_BUFF_SIZE - 1] = 0;

    HAL_TIM_PWM_Start_DMA(&htim2, TIM_CHANNEL_2, m_dmaBuffer, DMA_BUFF_SIZE);
    osDelay(pdMS_TO_TICKS(10));
}

void ControlScopeLight::NeoPixelSetColor(uint16_t index, uint8_t g, uint8_t r, uint8_t b)
{
    if (NUM_PIXELS > index)
    {
        m_pixel[index].color.g = g;
        m_pixel[index].color.r = r;
        m_pixel[index].color.b = b;
    }
}

// ***************************
// ***** Private Methods *****
// ***************************

void ControlScopeLight::TaskControlScopeLight()
{
    t_sensorPrev = GPIO_PIN_RESET;

    while (1)
    {
        if(m_touchSensorEnabled == true)
        {
            t_sensorCurr = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_10);

            if(t_sensorCurr == GPIO_PIN_SET && t_sensorPrev == GPIO_PIN_RESET)
            {
                m_led = !m_led;
                if(m_led)
                {
                    m_ledStateCurr = true;
                    ScopeLightsOn();

                } else
                {
                   m_ledStateCurr = false;
                   ScopeLightsOff();
                }
            } else
            {
            }

           t_sensorPrev = t_sensorCurr;
           vTaskDelay(pdMS_TO_TICKS(1));

        } else
        {
           vTaskDelay(pdMS_TO_TICKS(5));
        }
    }
}

void ControlScopeLight::EnableTouchSensor()
{
    m_touchSensorEnabled = true;
}

void ControlScopeLight::DisableTouchSensor()
{
    m_touchSensorEnabled = false;
    ScopeLightsOff();
}

void ControlScopeLight::TaskControlChamberLight()
{
    t_chamberSensorPrev = GPIO_PIN_RESET;
    uint32_t touchTime = 0;
    uint8_t tapCount = 0;
    uint32_t doubleTapTime = 400;

    while(1)
    {
        t_chamberSensorCurr = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_15);
        if(t_chamberSensorCurr == GPIO_PIN_SET && t_chamberSensorPrev == GPIO_PIN_RESET)
        {
            if (0 == tapCount)
            {
                touchTime = HAL_GetTick();
                tapCount++;
            }

            else
            {
               tapCount++;

               if (tapCount == 2)
               {
                   if ((HAL_GetTick() - touchTime) <= doubleTapTime)
                   {
                       IncreaseChamberBrightness();
                       tapCount = 0;
                   }
               }
            }
        } else
        {
        }

        if (tapCount == 1)
        {
            if ((HAL_GetTick() - touchTime) >= doubleTapTime)
            {
                m_chamberLed = !m_chamberLed;
                if(m_chamberLed)
                {
                    m_lightsStateCurr = true;
                    ChamberLightsOn(m_chamberRed, m_chamberGreen, m_chamberBlue);
                } else
                {
                    m_lightsStateCurr = false;
                    ChamberLightsOff();
                }
                tapCount = 0;
            }
        }

        t_chamberSensorPrev = t_chamberSensorCurr;
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}


void ControlScopeLight::IncreaseChamberBrightness()
{
    if(m_chamberRed == 0)
    {
        // do nothing
    }
    else
    {
        m_chamberRed = m_chamberRed + CHAMBER_RED_INCREMENT;
        if (CHAMBER_LIGHTS_BOUNDRY < m_chamberRed)
            m_chamberRed = CHAMBER_RED_THRESHOLD;
        ChamberLightsBoundries(m_chamberRed);
    }

    if(m_chamberGreen == 0)
    {
    }
    else
    {
        m_chamberGreen = m_chamberGreen + CHAMBER_GREEN_INCREMENT;
        if (CHAMBER_LIGHTS_BOUNDRY < m_chamberGreen)
            m_chamberGreen = CHAMBER_GREEN_THRESHOLD;
        ChamberLightsBoundries(m_chamberGreen);
    }

    if(m_chamberBlue == 0)
    {
    }
    else
    {
        m_chamberBlue = m_chamberBlue + CHAMBER_BLUE_INCREMENT;
        if (CHAMBER_LIGHTS_BOUNDRY < m_chamberBlue)
            m_chamberBlue = CHAMBER_BLUE_THRESHOLD;
        ChamberLightsBoundries(m_chamberBlue);

    }

    HAL_DAC_Start(&hdac1, DAC_CHANNEL_1);
    HAL_DAC_Start(&hdac1, DAC_CHANNEL_2);
    HAL_DAC_Start(&hdac2, DAC_CHANNEL_1);

    HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, m_chamberRed);
    HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_2, DAC_ALIGN_12B_R, m_chamberGreen);
    HAL_DAC_SetValue(&hdac2, DAC_CHANNEL_1, DAC_ALIGN_12B_R, m_chamberBlue);

//    dataSendSerial("  red = %d\n", m_chamberRed);
//    dataSendSerial("green = %d\n", m_chamberGreen);
//    dataSendSerial(" blue = %d\n", m_chamberBlue);
}

void ControlScopeLight::ScopeLightsBoundries(std::atomic<uint8_t>& border)
{
    if (0 > border)
    {
        border = SCOPE_LIGHTS_OFF;
    }

    if (border > SCOPE_LIGHTS_BOUNDRY)
    {
        border = SCOPE_LIGHTS_BOUNDRY;
    }
}

void ControlScopeLight::ChamberLightsBoundries(uint32_t& border)
{
    if (0 > border)
    {
        border = CHAMBER_LIGHTS_OFF;
    }

    if (border > CHAMBER_LIGHTS_BOUNDRY)
    {
        border = CHAMBER_LIGHTS_BOUNDRY;
    }
}

void ControlScopeLight::WhiteSettings(float percentage)
{
    if (percentage < 0)
    {
        percentage = 0;
    }

    else if (percentage > 100)
    {
        percentage =100;
    }

    const int R_values[ARRAY_SIZE] = {R_10, R_20, R_30, R_40, R_50, R_60, R_70, R_80, R_90, R_100};
    const int G_values[ARRAY_SIZE] = {G_10, G_20, G_30, G_40, G_50, G_60, G_70, G_80, G_90, G_100};
    const int B_values[ARRAY_SIZE] = {B_10, B_20, B_30, B_40, B_50, B_60, B_70, B_80, B_90, B_100};

    float stepSize = 100 / ARRAY_SIZE;
    float index = (percentage / stepSize) - 1.0;
    int intIndex = index * 10;
    int intIndexCheck = (int)intIndex;

    if(intIndex == intIndexCheck)
    {
        intIndex = intIndex / 10;

        int R = R_values[intIndex];
        int G = G_values[intIndex];
        int B = B_values[intIndex];

        m_chamberRed = R;
        m_chamberGreen = G;
        m_chamberBlue = B;

        ChamberLightsOn(m_chamberRed, m_chamberGreen, m_chamberBlue);

    }
    else
    {
        int index1 = (int) index;
        int index2 = index1 + 1;

        // y = mx + b

        float mR = (R_values[index1] - R_values[index2]) / 1; // rise over run
        float mG = (G_values[index1] - G_values[index2]) / 1;
        float mB = (B_values[index1] - B_values[index2]) / 1;

        float bR = index1 - (mR * index);
        float bG = index1 - (mG * index);
        float bB = index1 - (mB * index);

        float yR = (mR * index) + bR;
        float yG = (mG * index) + bG;
        float yB = (mB * index) + bB;

        m_chamberRed = yR;
        m_chamberGreen = yG;
        m_chamberBlue = yB;

        ChamberLightsOn(m_chamberRed, m_chamberGreen, m_chamberBlue);

    }
}

void ControlScopeLight::ScopeLightsOn()
{
    NeoPixelSetColor(0, m_scopeRed, m_scopeGreen, m_scopeBlue);
    NeoPixelSetColor(1, m_scopeRed, m_scopeGreen, m_scopeBlue);
    NeoPixelSetColor(2, m_scopeRed, m_scopeGreen, m_scopeBlue);
    NeoPixelSetColor(3, m_scopeRed, m_scopeGreen, m_scopeBlue);
    NeoPixelShow();
}

void ControlScopeLight::ChamberLightsOn(uint16_t r, uint16_t g, uint16_t b)
{
    HAL_DAC_Start(&hdac1, DAC_CHANNEL_1);
    HAL_DAC_Start(&hdac1, DAC_CHANNEL_2);
    HAL_DAC_Start(&hdac2, DAC_CHANNEL_1);

    HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, r);
    HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_2, DAC_ALIGN_12B_R, g);
    HAL_DAC_SetValue(&hdac2, DAC_CHANNEL_1, DAC_ALIGN_12B_R, b);

    m_chamberLed = true;
}

void ControlScopeLight::ScopeLightsOff()
{
    NeoPixelSetColor(0, 0, 0, 0);
    NeoPixelSetColor(1, 0, 0, 0);
    NeoPixelSetColor(2, 0, 0, 0);
    NeoPixelSetColor(3, 0, 0, 0);
    NeoPixelShow();
}

void ControlScopeLight::ChamberLightsOff()
{
    HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 0);
    HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_2, DAC_ALIGN_12B_R, 0);
    HAL_DAC_SetValue(&hdac2, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 0);

    m_chamberLed = false;
}
