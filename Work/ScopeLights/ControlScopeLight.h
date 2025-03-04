/**
 * @Author: Lojain Adly
 * @Date: 2023-09-29
*/

#ifndef CONTROL_SCOPE_LIGHT_H
#define CONTROL_SCOPE_LIGHT_H

#include "ControlBase.h"
#include <atomic>

// https://forum.digikey.com/t/controlling-neopixels-with-stm32/20527

#define ARR             213     // = (1.25e-6)/(1/(170e6))) = 212.5
#define NEO_PIXEL_0     69      // = (ARR + 1)(0.32) = 68.32
#define NEO_PIXEL_1     137     // = (ARR + 1)(0.64) = 136.64
#define NUM_PIXELS      4
#define PIXEL_BIT_SIZE  24
#define DMA_BUFF_SIZE   (NUM_PIXELS * PIXEL_BIT_SIZE) + 1

class ControlScopeLight: public ControlBase {

    public: 
        ControlScopeLight();
        virtual ~ControlScopeLight();
        virtual uint8_t StartTasks();
        
    protected:
        virtual bool Help(ControlCommandData& commandData);
        virtual bool ProcessCommands(ControlCommandData& commandData);
        virtual bool ProcessCommandScope(ControlCommandData& commandData);
        virtual bool ProcessCommandChamber(ControlCommandData& commandData);
        static void StartTaskControlScopeLight(void *arg);
        static void StartTaskControlChamberLight(void *arg);
        virtual bool ScopeLightsSet(ControlCommandData& commandData);
        virtual bool ChamberLightsSet(ControlCommandData& commandData);

        virtual void NeoPixelShow();
        virtual void NeoPixelSetColor(uint16_t index, uint8_t g, uint8_t r, uint8_t b);

    private: 
        virtual void TaskControlScopeLight();
        virtual void EnableTouchSensor();
        virtual void DisableTouchSensor();
        virtual void TaskControlChamberLight();
        virtual void IncreaseChamberBrightness();
        virtual void ScopeLightsBoundries(std::atomic<uint8_t>& border);
        virtual void ChamberLightsBoundries(uint32_t& border);
        virtual void WhiteSettings(float percentage);
        virtual void ScopeLightsOn();
        virtual void ChamberLightsOn(uint16_t r, uint16_t g, uint16_t b);
        virtual void ScopeLightsOff();
        virtual void ChamberLightsOff();

    private:
        bool                                    m_start;
        portBASE_TYPE                           m_taskStatus;
        bool                                    m_ledStateCurr;
        bool                                    m_lightsStateCurr;
        bool                                    m_ledStatePrev;
        bool                                    m_ledState;
        std::atomic<bool>                       m_led;
        std::atomic<bool>                       m_chamberLed;
        bool                                    t_chamberSensorCurr;
        bool                                    t_chamberSensorPrev;
        bool                                    t_sensorCurr;
        bool                                    t_sensorPrev;
        std::atomic<uint8_t>                    m_scopeRed;
        std::atomic<uint8_t>                    m_scopeGreen;
        std::atomic<uint8_t>                    m_scopeBlue;
        uint32_t                                m_chamberRed;
        uint32_t                                m_chamberGreen;
        uint32_t                                m_chamberBlue;
        uint32_t                                m_chamberWhite;
        bool                                    m_touchSensorEnabled;

        PixelRGB_t                              m_pixel[NUM_PIXELS];
        uint32_t                                m_dmaBuffer[DMA_BUFF_SIZE];
        uint32_t*                               m_pBuff;
        int                                     m_indexPixel;
        int                                     m_indexPixelBit;
        osThreadId_t                            m_scopeLightsDataTaskHandle;
        osThreadId_t                            m_chamberLightsDataTaskHandle;
        osThreadAttr_t                          m_scopeLightsDataTask_attributes;
        osThreadAttr_t                          m_chamberLightsDataTask_attributes;
};

#endif
