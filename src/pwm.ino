/*
  pwm.ino - FreeDs pwm functions
  Derivador de excedentes para ESP32 DEV Kit // Wifi Kit 32

  Inspired in opends+ (https://github.com/iqas/derivador)
  
  Copyright (C) 2020 Pablo Zerón (https://github.com/pablozg/freeds)

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

float targetEnergy; // Remove when debug is completed

void pwmControl()
{ 
  if (config.flags.debug2) { INFOV("PWMCONTROL()\n"); }

  if (config.flags.dimmerLowCost) { maxPwm = config.maxPwmLowCost; } 
  else { maxPwm = 1023;}

  // Check pwm_output
  if ((inverter.wgrid_control != inverter.wgrid) || config.flags.offGrid) // En caso de recepción de lectura, actualizamos el valor de invert_wgrid_control
  {
    inverter.wgrid_control = inverter.wgrid;
    // TODO: Filtrar los datos erroneos en ausencia de meter y ponerlos a 0.
    Error.VariacionDatos = false;
    timers.ErrorVariacionDatos = millis();
  }

  if (((millis() - timers.ErrorVariacionDatos) > config.maxErrorTime) && !Error.VariacionDatos)
  {
    INFOV(PSTR("PWM: Apagando PWM por superar el tiempo máximo en la recepción de los datos del consumo de Red\n"));
    Error.VariacionDatos = true;
    memset(&inverter, 0, sizeof(inverter));
    memset(&meter, 0, sizeof(meter));
  }

  //////////////////////////////// CONTROL MANUAL DEL PWM ////////////////////////////////
  if ((config.flags.pwmMan || Flags.pwmManAuto) && config.flags.pwmEnabled && Flags.pwmIsWorking)
  {
    static uint16_t tariffOffset = config.maxWattsTariff * 0.10; // Se calcula un 10% del total contratado, unos 345W sobre 3450W contratados.
    uint16_t maxTargetPwm = calculeTargetPwm(config.manualControlPWM);

    if (config.flags.changeGridSign ? inverter.wgrid < (config.maxWattsTariff - tariffOffset) : inverter.wgrid > -(config.maxWattsTariff - tariffOffset))
    {
      if (targetPwm < maxTargetPwm) {
        targetPwm += 8;
      } else { targetPwm = maxTargetPwm; }
      
      if (config.flags.dimmerLowCost && targetPwm < 210)
          targetPwm = 210;
    }
    
    if (config.flags.changeGridSign ? inverter.wgrid > config.maxWattsTariff : inverter.wgrid < -config.maxWattsTariff) {
      if (targetPwm >= 8) {
        targetPwm -= 8;
      } else { targetPwm = 0; }

      if (config.flags.dimmerLowCost && targetPwm < 210)
          targetPwm = 0;
    }

    invert_pwm = targetPwm;

    relay_control_man(false);
    INFOV("2- Max Target: %d, Target: %d, Invert_PWM: %d, Wgrid: %.02f, MaxWatts: %d, Offset: %d, Max-Offset: %d\n\n", maxTargetPwm, targetPwm, invert_pwm, inverter.wgrid, config.maxWattsTariff, tariffOffset, (config.maxWattsTariff - tariffOffset));
  }

  //////////////////////////////// CONTROL AUTOMÁTICO DEL PWM ////////////////////////////////
  if (config.flags.pwmEnabled && !config.flags.pwmMan && !Flags.pwmManAuto && !Error.VariacionDatos && Flags.pwmIsWorking)
  {
    if (config.flags.offGrid ? // Modo Off-grid
        (config.flags.offgridVoltage ? // True
          (config.flags.changeGridSign ? inverter.batteryWatts < config.pwmMin : inverter.batteryWatts > config.pwmMin) && meter.voltage >= config.batteryVoltage : // True
          (config.flags.changeGridSign ? inverter.batteryWatts < config.pwmMin : inverter.batteryWatts > config.pwmMin) && inverter.batterySoC >= config.soc // False
        ) : // Modo On-grid
        (config.flags.changeGridSign ? inverter.wgrid < config.pwmMin : inverter.wgrid > config.pwmMin) && inverter.batteryWatts >= config.battWatts // False
       )
    {
      if (config.flags.offGrid ?
          config.flags.changeGridSign ? inverter.batteryWatts < config.pwmMax : inverter.batteryWatts > config.pwmMax :
          config.flags.changeGridSign ? inverter.wgrid < config.pwmMax : inverter.wgrid > config.pwmMax
         ) 
      {
        
        // Falta adaptar el código del objetivo a las configuraciones de aislada y baterías.
        // Como idea pasando de manual a automatico el pid según se vayan cumpliendo las condiciones
        // Crear función o condición para cuando se pase a manual el invert_pwm esté siempre a 0.
        calcTargetPID();
      }
    }
    else if (config.flags.offGrid ?
              (config.flags.offgridVoltage ?
                (config.flags.changeGridSign ? inverter.batteryWatts > config.pwmMax : inverter.batteryWatts < config.pwmMax) || meter.voltage < (config.batteryVoltage - 0.10) : // True
                (config.flags.changeGridSign ? inverter.batteryWatts > config.pwmMax : inverter.batteryWatts < config.pwmMax) || inverter.batterySoC < config.soc // False
              ) :
              (config.flags.changeGridSign ? inverter.wgrid > config.pwmMax : inverter.wgrid < config.pwmMax) || inverter.batteryWatts < config.battWatts
            )
    {
      calcTargetPID();
    }
  }

  writePwmValue(invert_pwm);

  /////////////////////////////////////////////////////////////// SALIDAS POR PORCENTAJE ////////////////////////////////////////////////////////////////////////////////

  // Start at defined pwm value

  if (!config.flags.pwmMan && config.flags.pwmEnabled)
  {
    if (!digitalRead(PIN_RL1) && Flags.RelayTurnOn && (pwmValue >= config.R01Min))
    {
      // Start relay1
      digitalWrite(PIN_RL1, HIGH);
      INFOV("Encendido por %% Salida 1\n");
      Flags.RelayTurnOn = false;
      Flags.Relay01Auto = true;
      if (config.flags.mqtt && !Error.ConexionMqtt && config.wversion != SOLAX_V2_LOCAL)
        publisher(config.R01_mqtt, digitalRead(PIN_RL1) ? "ON" : "OFF");
    }

    if (!digitalRead(PIN_RL2) && Flags.RelayTurnOn && (pwmValue >= config.R02Min))
    {
      // Start relay2
      digitalWrite(PIN_RL2, HIGH);
      INFOV("Encendido por %% Salida 2\n");
      Flags.RelayTurnOn = false;
      Flags.Relay02Auto = true;
      if (config.flags.mqtt && !Error.ConexionMqtt && config.wversion != SOLAX_V2_LOCAL)
        publisher(config.R02_mqtt, digitalRead(PIN_RL2) ? "ON" : "OFF");
    }

    if (!digitalRead(PIN_RL3) && Flags.RelayTurnOn && (pwmValue >= config.R03Min))
    {
      // Start relay3
      digitalWrite(PIN_RL3, HIGH);
      INFOV("Encendido por %% Salida 3\n");
      Flags.RelayTurnOn = false;
      Flags.Relay03Auto = true;
      if (config.flags.mqtt && !Error.ConexionMqtt && config.wversion != SOLAX_V2_LOCAL)
        publisher(config.R03_mqtt, digitalRead(PIN_RL3) ? "ON" : "OFF");
    }

    if (!digitalRead(PIN_RL4) && Flags.RelayTurnOn && (pwmValue >= config.R04Min))
    {
      // Start relay4
      digitalWrite(PIN_RL4, HIGH);
      INFOV("Encendido por %% Salida 4\n");
      Flags.RelayTurnOn = false;
      Flags.Relay04Auto = true;
      if (config.flags.mqtt && !Error.ConexionMqtt && config.wversion != SOLAX_V2_LOCAL)
        publisher(config.R04_mqtt, digitalRead(PIN_RL4) ? "ON" : "OFF");
    }
  }

  // Stop at defined porcent of power

  if (!Flags.RelayTurnOff)
  {
    if (!(Flags.Relay04Man || config.relaysFlags.R04Man) && digitalRead(PIN_RL4) && !Flags.RelayTurnOff && (pwmValue <= ((config.R04Min - 10) < 0 ? 0 : (config.R04Min - 10))) && config.R04Min != 999)
    {
      digitalWrite(PIN_RL4, LOW);
      INFOV("Apagado por %% Salida 4\n");
      Flags.RelayTurnOff = true;
      Flags.Relay04Auto = false;
      if (config.flags.mqtt && !Error.ConexionMqtt && config.wversion != SOLAX_V2_LOCAL)
      {
        publisher(config.R04_mqtt, digitalRead(PIN_RL4) ? "ON" : "OFF");
      }
    }
    if (!(Flags.Relay03Man || config.relaysFlags.R03Man) && digitalRead(PIN_RL3) && !Flags.RelayTurnOff && (pwmValue <= ((config.R03Min - 10) < 0 ? 0 : (config.R03Min - 10))) && config.R03Min != 999)
    {
      digitalWrite(PIN_RL3, LOW);
      INFOV("Apagado por %% Salida 3\n");
      Flags.RelayTurnOff = true;
      Flags.Relay03Auto = false;
      if (config.flags.mqtt && !Error.ConexionMqtt && config.wversion != SOLAX_V2_LOCAL)
      {
        publisher(config.R03_mqtt, digitalRead(PIN_RL3) ? "ON" : "OFF");
      }
    }
    if (!(Flags.Relay02Man || config.relaysFlags.R02Man) && digitalRead(PIN_RL2) && !Flags.RelayTurnOff && (pwmValue <= ((config.R02Min - 10) < 0 ? 0 : (config.R02Min - 10))) && config.R02Min != 999)
    {
      digitalWrite(PIN_RL2, LOW);
      INFOV("Apagado por %% Salida 2\n");
      Flags.RelayTurnOff = true;
      Flags.Relay02Auto = false;
      if (config.flags.mqtt && !Error.ConexionMqtt && config.wversion != SOLAX_V2_LOCAL)
      {
        publisher(config.R02_mqtt, digitalRead(PIN_RL2) ? "ON" : "OFF");
      }
    }
    if (!(Flags.Relay01Man || config.relaysFlags.R01Man) && digitalRead(PIN_RL1) && !Flags.RelayTurnOff && (pwmValue <= ((config.R01Min - 10) < 0 ? 0 : (config.R01Min - 10))) && config.R01Min != 999)
    {
      digitalWrite(PIN_RL1, LOW);
      INFOV("Apagado por %% Salida 1\n");
      Flags.RelayTurnOff = true;
      Flags.Relay01Auto = false;
      if (config.flags.mqtt && !Error.ConexionMqtt && config.wversion != SOLAX_V2_LOCAL)
      {
        publisher(config.R01_mqtt, digitalRead(PIN_RL1) ? "ON" : "OFF");
      }
    }
  }

  //////////////////////////////////////////////////////////////////////// SALIDAS POR POTENCIA ////////////////////////////////////////////////////////////////////////////////////

  // Start at defined power on

  if (!config.flags.pwmMan && config.flags.pwmEnabled && (pwmValue >= config.autoControlPWM))
  {
    //if (!digitalRead(PIN_RL1) && Flags.RelayTurnOn && config.R01Min == 999 && (inverter.wgrid > config.R01PotOn))
    if (!digitalRead(PIN_RL1) && Flags.RelayTurnOn && config.R01Min == 999 && (config.flags.changeGridSign ? inverter.wgrid < config.R01PotOn : inverter.wgrid > config.R01PotOn))
    {
      // Start relay1
      digitalWrite(PIN_RL1, HIGH);
      INFOV("Encendido por W Salida 1\n");
      Flags.Relay01Auto = true;
      Flags.RelayTurnOn = false;
      if (config.flags.mqtt && !Error.ConexionMqtt && config.wversion != SOLAX_V2_LOCAL)
        publisher(config.R01_mqtt, digitalRead(PIN_RL1) ? "ON" : "OFF");
    }
    if (!digitalRead(PIN_RL2) && Flags.RelayTurnOn && config.R02Min == 999 && (config.flags.changeGridSign ? inverter.wgrid < config.R02PotOn : inverter.wgrid > config.R02PotOn))
    {
      // Start relay2
      digitalWrite(PIN_RL2, HIGH);
      INFOV("Encendido por W Salida 2\n");
      Flags.Relay02Auto = true;
      Flags.RelayTurnOn = false;
      if (config.flags.mqtt && !Error.ConexionMqtt && config.wversion != SOLAX_V2_LOCAL)
        publisher(config.R02_mqtt, digitalRead(PIN_RL2) ? "ON" : "OFF");
    }
    if (!digitalRead(PIN_RL3) && Flags.RelayTurnOn && config.R03Min == 999 && (config.flags.changeGridSign ? inverter.wgrid < config.R03PotOn : inverter.wgrid > config.R03PotOn))
    {
      // Start relay3
      digitalWrite(PIN_RL3, HIGH);
      INFOV("Encendido por W Salida 3\n");
      Flags.Relay03Auto = true;
      Flags.RelayTurnOn = false;
      if (config.flags.mqtt && !Error.ConexionMqtt && config.wversion != SOLAX_V2_LOCAL)
        publisher(config.R03_mqtt, digitalRead(PIN_RL3) ? "ON" : "OFF");
    }
    if (!digitalRead(PIN_RL4) && Flags.RelayTurnOn && config.R04Min == 999 && (config.flags.changeGridSign ? inverter.wgrid < config.R04PotOn : inverter.wgrid > config.R04PotOn))
    {
      // Start relay4
      digitalWrite(PIN_RL4, HIGH);
      INFOV("Encendido por W Salida 4\n");
      Flags.Relay04Auto = true;
      Flags.RelayTurnOn = false;
      if (config.flags.mqtt && !Error.ConexionMqtt && config.wversion != SOLAX_V2_LOCAL)
        publisher(config.R04_mqtt, digitalRead(PIN_RL4) ? "ON" : "OFF");
    }
  }

  // Stop at defined power off

  // if (!(Flags.Relay04Man || config.relaysFlags.R04Man) && digitalRead(PIN_RL4) && !Flags.RelayTurnOff && config.R04Min == 999 && (inverter.wgrid < config.R04PotOff))
  if (!(Flags.Relay04Man || config.relaysFlags.R04Man) && digitalRead(PIN_RL4) && !Flags.RelayTurnOff && config.R04Min == 999 && (config.flags.changeGridSign ? inverter.wgrid > config.R04PotOff : inverter.wgrid < config.R04PotOff))
  {
    // Start relay4
    digitalWrite(PIN_RL4, LOW);
    INFOV("Apagado por W Salida 4\n");
    Flags.Relay04Auto = false;
    Flags.RelayTurnOff = true;
    if (config.flags.mqtt && !Error.ConexionMqtt && config.wversion != SOLAX_V2_LOCAL)
      publisher(config.R04_mqtt, digitalRead(PIN_RL4) ? "ON" : "OFF");
  }

  if (!(Flags.Relay03Man || config.relaysFlags.R03Man) && digitalRead(PIN_RL3) && !Flags.RelayTurnOff && config.R03Min == 999 && (config.flags.changeGridSign ? inverter.wgrid > config.R03PotOff : inverter.wgrid < config.R03PotOff))
  {
    // Start relay3
    digitalWrite(PIN_RL3, LOW);
    INFOV("Apagado por W Salida 3\n");
    Flags.Relay03Auto = false;
    Flags.RelayTurnOff = true;
    if (config.flags.mqtt && !Error.ConexionMqtt && config.wversion != SOLAX_V2_LOCAL)
      publisher(config.R03_mqtt, digitalRead(PIN_RL3) ? "ON" : "OFF");
  }

  if (!(Flags.Relay02Man || config.relaysFlags.R02Man) && digitalRead(PIN_RL2) && !Flags.RelayTurnOff && config.R02Min == 999 && (config.flags.changeGridSign ? inverter.wgrid > config.R02PotOff : inverter.wgrid < config.R02PotOff))
  {
    // Start relay2
    digitalWrite(PIN_RL2, LOW);
    INFOV("Apagado por W Salida 2\n");
    Flags.Relay02Auto = false;
    Flags.RelayTurnOff = true;
    if (config.flags.mqtt && !Error.ConexionMqtt && config.wversion != SOLAX_V2_LOCAL)
      publisher(config.R02_mqtt, digitalRead(PIN_RL2) ? "ON" : "OFF");
  }

  if (!(Flags.Relay01Man || config.relaysFlags.R01Man) && digitalRead(PIN_RL1) && !Flags.RelayTurnOff && config.R01Min == 999 && (config.flags.changeGridSign ? inverter.wgrid > config.R01PotOff : inverter.wgrid < config.R01PotOff))
  {
    // Start relay1
    digitalWrite(PIN_RL1, LOW);
    INFOV("Apagado por W Salida 1\n");
    Flags.Relay01Auto = false;
    Flags.RelayTurnOff = true;
    if (config.flags.mqtt && !Error.ConexionMqtt && config.wversion != SOLAX_V2_LOCAL)
      publisher(config.R01_mqtt, digitalRead(PIN_RL1) ? "ON" : "OFF");
  }

  if (!Flags.RelayTurnOn && xTimerIsTimerActive(relayOnTimer) == pdFALSE)
  {
    xTimerStart(relayOnTimer, 0);
  }

  if (Flags.RelayTurnOff && xTimerIsTimerActive(relayOffTimer) == pdFALSE)
  {
    xTimerStart(relayOffTimer, 0);
  } 
}

void enableRelay(void)
{
  Flags.RelayTurnOn = true;
  xTimerStop(relayOnTimer, 0);
}

void disableRelay(void)
{
  Flags.RelayTurnOff = false;
  xTimerStop(relayOffTimer, 0);
}

void calcPwmProgressBar()
{
  // Arreglos necesarios para mostrar el % correcto en caso de usar el dimmer low cost afectado por el fallo del 20%
  if (config.flags.dimmerLowCost) {
    if (invert_pwm > 0) { pwmValue = round(((invert_pwm - 210) * 100.0) / (config.maxPwmLowCost - 210.0)); }
    else pwmValue = 0;
  } else {  
    pwmValue = round((invert_pwm * 100.0) / 1023.0);
  }
}

void relay_control_man(boolean forceOFF)
{

  if (config.flags.debug2) { INFOV("relay_control_man()\n"); }

  // if (!(Flags.Relay01Man || config.relaysFlags.R01Man) && !Flags.Relay01Auto)
  //   digitalWrite(PIN_RL1, LOW);
  // if (!(Flags.Relay02Man || config.relaysFlags.R02Man) && !Flags.Relay02Auto)
  //   digitalWrite(PIN_RL2, LOW);
  // if (!(Flags.Relay03Man || config.relaysFlags.R03Man) && !Flags.Relay03Auto)
  //   digitalWrite(PIN_RL3, LOW);
  // if (!(Flags.Relay04Man || config.relaysFlags.R04Man) && !Flags.Relay04Auto)
  //   digitalWrite(PIN_RL4, LOW);

  // Se activan las salidas seleccionadas manualmente si no lo están ya
  if ((Flags.Relay01Man || config.relaysFlags.R01Man) && !digitalRead(PIN_RL1)) { digitalWrite(PIN_RL1, HIGH); INFOV("Relay 1 Forced On\n");}
  if ((Flags.Relay02Man || config.relaysFlags.R02Man) && !digitalRead(PIN_RL2)) { digitalWrite(PIN_RL2, HIGH); INFOV("Relay 2 Forced On\n");}
  if ((Flags.Relay03Man || config.relaysFlags.R03Man) && !digitalRead(PIN_RL3)) { digitalWrite(PIN_RL3, HIGH); INFOV("Relay 3 Forced On\n");}
  if ((Flags.Relay04Man || config.relaysFlags.R04Man) && !digitalRead(PIN_RL4)) { digitalWrite(PIN_RL4, HIGH); INFOV("Relay 4 Forced On\n");}

  // Se fuerza el apagado de todas las salidas
    if (forceOFF || !config.flags.pwmEnabled)
  {
    if (digitalRead(PIN_RL1) && !Flags.Relay01Man && !config.relaysFlags.R01Man) { digitalWrite(PIN_RL1, LOW); INFOV("Relay 1 Forced Off\n"); }
    if (digitalRead(PIN_RL2) && !Flags.Relay02Man && !config.relaysFlags.R02Man) { digitalWrite(PIN_RL2, LOW); INFOV("Relay 2 Forced Off\n"); }
    if (digitalRead(PIN_RL3) && !Flags.Relay03Man && !config.relaysFlags.R03Man) { digitalWrite(PIN_RL3, LOW); INFOV("Relay 3 Forced Off\n"); }
    if (digitalRead(PIN_RL4) && !Flags.Relay04Man && !config.relaysFlags.R04Man) { digitalWrite(PIN_RL4, LOW); INFOV("Relay 4 Forced Off\n"); }
  }
}

// PWM Functions
void down_pwm(const char *message)
{
  myPID.SetMode(MANUAL);
    PIDOutput = 0;
    invert_pwm = 0;
    pwmValue = 0;
    ledcWrite(2, 0); // Hard shutdown
    dac_output_disable(DAC_CHANNEL_2);
    INFOV("PWM disabled\n");
    calcPwmProgressBar();

  if (invert_pwm != last_invert_pwm)
  {
    if (config.flags.debug1) { INFOV(PSTR(message)); }
    last_invert_pwm = invert_pwm;
  }

  relay_control_man(true);
}

void calcTargetPID(void)
{
  if (!config.flags.pwmMan && !Flags.pwmManAuto) { 
    Setpoint = config.flags.changeGridSign ? config.pwmMin + ((config.pwmMax - config.pwmMin) / 2) : config.pwmMax + ((config.pwmMin - config.pwmMax) / 2);
  }
}

void writePwmValue(uint16_t value)
{
  // if (config.flags.dimmerLowCost && value > 1023) { value -= 1023; } 
  if (config.flags.dimmerLowCost) {
    if (value <= 210) 
      value = 0;
    if (value > 1023) 
      value -= 1023;
  } 
  
  ledcWrite(2, value);
  dac_output_voltage(DAC_CHANNEL_2, constrain((value / 4), 0, 255));
  calcPwmProgressBar();
}

uint16_t calculeTargetPwm(uint16_t targetValue)
{
  uint16_t maxTargetPwm;

  if (config.flags.dimmerLowCost) { 
    maxTargetPwm = ((((config.maxPwmLowCost - 210) * targetValue) / 100) + 210);
    if (maxTargetPwm <= 210) { maxTargetPwm = 0; }
  } else { maxTargetPwm = (1023 * targetValue) / 100; }

  return maxTargetPwm;
}

// void upPwmClamp(void)
// {
//   uint16_t maxPwm;
//   static uint16_t tariffOffset = config.maxWattsTariff * 0.10; // Se calcula un 10% del total contratado, unos 345W sobre 3450W contratados.
  
//   if (config.flags.pwmMan || Flags.pwmManAuto) {
//     targetEnergy = config.flags.changeGridSign ? inverter.currentCalcWatts + ((config.maxWattsTariff - tariffOffset) - inverter.wgrid) : inverter.currentCalcWatts + ((config.maxWattsTariff - tariffOffset) + inverter.wgrid);
//   } else {
//     targetEnergy = config.flags.changeGridSign ? inverter.currentCalcWatts + (-inverter.wgrid - ((config.pwmMin - config.pwmMax) / 3)) : inverter.currentCalcWatts + (inverter.wgrid - ((config.pwmMin - config.pwmMax) / 3)); // hasta ahora /2
//   }

//   // Si el pwm está al máximo salimos inmediatamente
//   if (config.flags.dimmerLowCost) { 
//     maxPwm = ((((config.maxPwmLowCost - 210) * 100) / 100) + 210);
//   } else { maxPwm = 1023; }
  
//   if (invert_pwm >= maxPwm)
//     return;

//   Setpoint = targetEnergy;
// }

// void downPwmClamp(void)
// { 
//   if (config.flags.pwmMan || Flags.pwmManAuto) {
//     targetEnergy = config.flags.changeGridSign ? inverter.currentCalcWatts - (inverter.wgrid - config.maxWattsTariff) : inverter.currentCalcWatts - (-inverter.wgrid - config.maxWattsTariff);
//   } else {
//     targetEnergy = config.flags.changeGridSign ? inverter.currentCalcWatts - (inverter.wgrid + ((config.pwmMin - config.pwmMax) * 0.8)) : inverter.currentCalcWatts + (inverter.wgrid - ((config.pwmMin - config.pwmMax) * 0.8));
//   }

//   // Si el pwm está a 0 salimos inmediatamente
//   if (invert_pwm == 0)
//     return;

//   Setpoint = targetEnergy;
// }

