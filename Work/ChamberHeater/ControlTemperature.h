/**
 * @Author: lojainadly
 * @Date:   2025-01-20
 * @Last modified by: ekupitz
 * @Last modified time: 2025-02-19 */

 #ifndef CONTROL_TEMPERATURE_H
 #define  CONTROL_TEMPERATURE_H

 #include <ControlCommandData.h>
 #include "ControlBase.h"
 #include "ControlIncludes.h"
 #include "Max31865.h"
 #include <atomic>

class ControlTemperature : public ControlBase {
  public:
    ControlTemperature();
    virtual ~ControlTemperature();

    virtual uint8_t StartTasks();
    virtual float GetTemp();
    virtual float GetGoalTemp();

  protected:
    virtual bool ProcessCommands(ControlCommandData& commandData);
    virtual bool Help(ControlCommandData& commandData);
    virtual bool Temp(ControlCommandData& commandData);
    virtual bool Heat(ControlCommandData& commandData);

  private:
    static void StartTaskControlTemperature(void *arg);
    static void StartTaskControlHeater(void *arg);

    virtual void TaskControlTemperature();
    virtual void TaskControlHeater();

    virtual void UpdateTemp();
    virtual void HeatOn();
    virtual void HeatOff();
    virtual void TouchSensor();
    virtual void TemperatureStatus();

  private:
    Max31865_t                max31865;
    int                       m_ThreadID;
    portBASE_TYPE             m_taskStatus;
    std::atomic<float>        m_currentTemp;
    std::atomic<float>        m_setTemp;
    std::atomic<float>        m_tempOffset;
    bool                      m_start;
    bool                      m_heat;
    bool                      m_tempOn;
    bool                      m_tempSet;
    bool                      read;
    float                     temp;
    
    osThreadId_t              m_taskHeater;
    osThreadId_t              m_taskTemp;
    osThreadAttr_t            m_controlHeaterTask_attributes;
    osThreadAttr_t            m_controlTempTask_attributes;
};

 #endif
