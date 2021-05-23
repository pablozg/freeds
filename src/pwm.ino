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

void pwmControl()
{ 
  if (config.flags.debug2) { INFOV("PWMCONTROL()\n"); }

  // En caso de recepción de lectura distinta a la almacenada la actualizamos con el nuevo valor
  if ((inverter.wgrid_control != inverter.wgrid) || config.flags.offGrid)
  {
    inverter.wgrid_control = inverter.wgrid;
    // TODO: Filtrar los datos erroneos en ausencia de meter y ponerlos a 0.
    Error.VariacionDatos = false;
    timers.ErrorVariacionDatos = millis();
  }

  // Same data along error time
  if (((millis() - timers.ErrorVariacionDatos) > config.maxErrorTime) && !Error.VariacionDatos)
  {
    INFOV(PSTR("PWM: Apagando PWM por superar el tiempo máximo en la variación de los datos del consumo de Red\n"));
    memset(&inverter, 0, sizeof(inverter));
    memset(&meter, 0, sizeof(meter));
    Error.VariacionDatos = true;
  }

  // Lost connection error
  if (((millis() - timers.ErrorRecepcionDatos) > config.maxErrorTime) && !Error.RecepcionDatos)
  {
    INFOV(PSTR("PWM: Apagando PWM por error en la conexión con la fuente de datos\n"));
    memset(&inverter, 0, sizeof(inverter));
    memset(&meter, 0, sizeof(meter));
    Error.RecepcionDatos = true;
  }

  if ((Error.RecepcionDatos || Error.VariacionDatos || !Flags.pwmIsWorking) && myPID.GetMode() == PID::AUTOMATIC) { shutdownPwm(true, "PWM: Down by security reasons\n"); }

  //////////////////////////////// CONTROL MANUAL DEL PWM ////////////////////////////////
  if ((config.flags.pwmMan || Flags.pwmManAuto) && config.flags.pwmEnabled && Flags.pwmIsWorking)
  {
    static uint16_t tariffOffset = config.maxWattsTariff * 0.10; // Se calcula un 10% del total contratado, unos 345W sobre 3450W contratados.
    uint16_t maxTargetPwm = calculeTargetPwm(config.manualControlPWM);

    if ((config.flags.changeGridSign ? inverter.wgrid < (config.maxWattsTariff - tariffOffset) : inverter.wgrid > -(config.maxWattsTariff - tariffOffset)) && pwm.targetPwm < maxTargetPwm)
    {
      if (pwm.targetPwm < maxTargetPwm - 8) {
        pwm.targetPwm += 8;
      } else { pwm.targetPwm = maxTargetPwm; }
      
      if (config.flags.dimmerLowCost && pwm.targetPwm < 210)
          pwm.targetPwm = 210;
    }
    
    if ((config.flags.changeGridSign ? inverter.wgrid > config.maxWattsTariff : inverter.wgrid < -config.maxWattsTariff) || pwm.targetPwm > maxTargetPwm) {
      if (pwm.targetPwm >= 8) {
        pwm.targetPwm -= 8;
      } else { pwm.targetPwm = 0; }

      if (config.flags.dimmerLowCost && pwm.targetPwm < 210)
          pwm.targetPwm = 0;
    }

    if (pwm.invert_pwm != pwm.targetPwm) { pwm.invert_pwm = pwm.targetPwm; writePwmValue(pwm.invert_pwm); }

    // relayManualControl(false);
    // INFOV("Manual- Max Target: %d, Target: %d, Invert_PWM: %d, Wgrid: %.02f, MaxWatts: %d, Offset: %d, Max-Offset: %d\n\n", maxTargetPwm, pwm.targetPwm, pwm.invert_pwm, inverter.wgrid, config.maxWattsTariff, tariffOffset, (config.maxWattsTariff - tariffOffset));
  }

  //////////////////////////////// CONTROL AUTOMÁTICO DEL PWM ////////////////////////////////
  if (config.flags.pwmEnabled && !config.flags.pwmMan && !Flags.pwmManAuto && !Error.VariacionDatos && !Error.RecepcionDatos && Flags.pwmIsWorking)
  {
    // INFOV("pwm.invert_pwm: %s, wgrid: %s, comparacion: %s, , comparacion_sin_signo: %s\n", pwm.invert_pwm == 0 ? "true" : "false", inverter.wgrid < config.potTarget ? "true" : "false", (pwm.invert_pwm == 0 && (config.flags.changeGridSign ? inverter.wgrid > config.potTarget : inverter.wgrid < config.potTarget)) ? "true" : "false", (pwm.invert_pwm == 0 && inverter.wgrid < config.potTarget) ? "true" : "false");
    if (config.flags.offGrid ? // Modo Off-grid
        (config.flags.offgridVoltage ? // True
          (myPID.GetMode() == PID::MANUAL && inverter.batteryWatts > config.potTarget && meter.voltage >= config.batteryVoltage) : // True
          (myPID.GetMode() == PID::MANUAL && inverter.batteryWatts > config.potTarget && inverter.batterySoC >= config.soc) // False
        ) : // Modo On-grid
          (myPID.GetMode() == PID::MANUAL && (config.flags.changeGridSign ? inverter.wgrid < config.potTarget : inverter.wgrid > config.potTarget) && inverter.batteryWatts >= config.battWatts) // False
       )
    {
      myPID.SetMode(PID::AUTOMATIC);
      config.flags.offGrid ? myPID.SetControllerDirection(PID::REVERSE) : config.flags.changeGridSign ? myPID.SetControllerDirection(PID::DIRECT) : myPID.SetControllerDirection(PID::REVERSE);
      Setpoint = config.potTarget;
    }
    else if (config.flags.offGrid ?
              (config.flags.offgridVoltage ?
                myPID.GetMode() == PID::AUTOMATIC && (meter.voltage < (config.batteryVoltage - config.voltageOffset) || (pwm.invert_pwm == 0 && inverter.batteryWatts < config.potTarget)): // True
                myPID.GetMode() == PID::AUTOMATIC && (inverter.batterySoC < config.soc || (pwm.invert_pwm == 0 && inverter.batteryWatts < config.potTarget)) // False
              ) :
                myPID.GetMode() == PID::AUTOMATIC && (inverter.batteryWatts < config.battWatts || (pwm.invert_pwm == 0 && (config.flags.changeGridSign ? inverter.wgrid > config.potTarget : inverter.wgrid < config.potTarget)))
            )
    {
      shutdownPwm(false);
    }
  }

  /////////////////////////////////////////////////////////////// SALIDAS POR PORCENTAJE ////////////////////////////////////////////////////////////////////////////////

  // Start at defined pwm value

  if (!config.flags.pwmMan && config.flags.pwmEnabled)
  {
    if (!digitalRead(PIN_RL1) && Flags.RelayTurnOn && (pwm.pwmValue >= config.R01Min))
    {
      // Start relay1
      digitalWrite(PIN_RL1, HIGH);
      INFOV("Encendido por %% Salida 1\n");
      Flags.RelayTurnOn = false;
      Flags.Relay01Auto = true;
      if (config.flags.mqtt && !Error.ConexionMqtt && strcmp("5.8.8.8", config.sensor_ip) != 0)
        publisher(config.R01_mqtt, digitalRead(PIN_RL1) ? "ON" : "OFF");
    }

    if (!digitalRead(PIN_RL2) && Flags.RelayTurnOn && (pwm.pwmValue >= config.R02Min))
    {
      // Start relay2
      digitalWrite(PIN_RL2, HIGH);
      INFOV("Encendido por %% Salida 2\n");
      Flags.RelayTurnOn = false;
      Flags.Relay02Auto = true;
      if (config.flags.mqtt && !Error.ConexionMqtt && strcmp("5.8.8.8", config.sensor_ip) != 0)
        publisher(config.R02_mqtt, digitalRead(PIN_RL2) ? "ON" : "OFF");
    }

    if (!digitalRead(PIN_RL3) && Flags.RelayTurnOn && (pwm.pwmValue >= config.R03Min))
    {
      // Start relay3
      digitalWrite(PIN_RL3, HIGH);
      INFOV("Encendido por %% Salida 3\n");
      Flags.RelayTurnOn = false;
      Flags.Relay03Auto = true;
      if (config.flags.mqtt && !Error.ConexionMqtt && strcmp("5.8.8.8", config.sensor_ip) != 0)
        publisher(config.R03_mqtt, digitalRead(PIN_RL3) ? "ON" : "OFF");
    }

    if (!digitalRead(PIN_RL4) && Flags.RelayTurnOn && (pwm.pwmValue >= config.R04Min))
    {
      // Start relay4
      digitalWrite(PIN_RL4, HIGH);
      INFOV("Encendido por %% Salida 4\n");
      Flags.RelayTurnOn = false;
      Flags.Relay04Auto = true;
      if (config.flags.mqtt && !Error.ConexionMqtt && strcmp("5.8.8.8", config.sensor_ip) != 0)
        publisher(config.R04_mqtt, digitalRead(PIN_RL4) ? "ON" : "OFF");
    }
  }

  // Stop at defined porcent of power

  if (!Flags.RelayTurnOff)
  {
    if (!(Flags.Relay04Man || config.relaysFlags.R04Man) && digitalRead(PIN_RL4) && !Flags.RelayTurnOff && (pwm.pwmValue <= ((config.R04Min - 10) < 0 ? 0 : (config.R04Min - 10))) && config.R04Min != 999)
    {
      digitalWrite(PIN_RL4, LOW);
      INFOV("Apagado por %% Salida 4\n");
      Flags.RelayTurnOff = true;
      Flags.Relay04Auto = false;
      if (config.flags.mqtt && !Error.ConexionMqtt && strcmp("5.8.8.8", config.sensor_ip) != 0)
      {
        publisher(config.R04_mqtt, digitalRead(PIN_RL4) ? "ON" : "OFF");
      }
    }
    if (!(Flags.Relay03Man || config.relaysFlags.R03Man) && digitalRead(PIN_RL3) && !Flags.RelayTurnOff && (pwm.pwmValue <= ((config.R03Min - 10) < 0 ? 0 : (config.R03Min - 10))) && config.R03Min != 999)
    {
      digitalWrite(PIN_RL3, LOW);
      INFOV("Apagado por %% Salida 3\n");
      Flags.RelayTurnOff = true;
      Flags.Relay03Auto = false;
      if (config.flags.mqtt && !Error.ConexionMqtt && strcmp("5.8.8.8", config.sensor_ip) != 0)
      {
        publisher(config.R03_mqtt, digitalRead(PIN_RL3) ? "ON" : "OFF");
      }
    }
    if (!(Flags.Relay02Man || config.relaysFlags.R02Man) && digitalRead(PIN_RL2) && !Flags.RelayTurnOff && (pwm.pwmValue <= ((config.R02Min - 10) < 0 ? 0 : (config.R02Min - 10))) && config.R02Min != 999)
    {
      digitalWrite(PIN_RL2, LOW);
      INFOV("Apagado por %% Salida 2\n");
      Flags.RelayTurnOff = true;
      Flags.Relay02Auto = false;
      if (config.flags.mqtt && !Error.ConexionMqtt && strcmp("5.8.8.8", config.sensor_ip) != 0)
      {
        publisher(config.R02_mqtt, digitalRead(PIN_RL2) ? "ON" : "OFF");
      }
    }
    if (!(Flags.Relay01Man || config.relaysFlags.R01Man) && digitalRead(PIN_RL1) && !Flags.RelayTurnOff && (pwm.pwmValue <= ((config.R01Min - 10) < 0 ? 0 : (config.R01Min - 10))) && config.R01Min != 999)
    {
      digitalWrite(PIN_RL1, LOW);
      INFOV("Apagado por %% Salida 1\n");
      Flags.RelayTurnOff = true;
      Flags.Relay01Auto = false;
      if (config.flags.mqtt && !Error.ConexionMqtt && strcmp("5.8.8.8", config.sensor_ip) != 0)
      {
        publisher(config.R01_mqtt, digitalRead(PIN_RL1) ? "ON" : "OFF");
      }
    }
  }

  //////////////////////////////////////////////////////////////////////// SALIDAS POR POTENCIA ////////////////////////////////////////////////////////////////////////////////////

  // Start at defined power on

  if (!config.flags.pwmMan && config.flags.pwmEnabled && (pwm.pwmValue >= config.autoControlPWM))
  {
    if (!digitalRead(PIN_RL1) && Flags.RelayTurnOn && config.R01Min == 999 && (config.flags.changeGridSign ? inverter.wgrid < config.R01PotOn : inverter.wgrid > config.R01PotOn))
    {
      // Start relay1
      digitalWrite(PIN_RL1, HIGH);
      INFOV("Encendido por W Salida 1\n");
      Flags.Relay01Auto = true;
      Flags.RelayTurnOn = false;
      if (config.flags.mqtt && !Error.ConexionMqtt && strcmp("5.8.8.8", config.sensor_ip) != 0)
        publisher(config.R01_mqtt, digitalRead(PIN_RL1) ? "ON" : "OFF");
    }
    if (!digitalRead(PIN_RL2) && Flags.RelayTurnOn && config.R02Min == 999 && (config.flags.changeGridSign ? inverter.wgrid < config.R02PotOn : inverter.wgrid > config.R02PotOn))
    {
      // Start relay2
      digitalWrite(PIN_RL2, HIGH);
      INFOV("Encendido por W Salida 2\n");
      Flags.Relay02Auto = true;
      Flags.RelayTurnOn = false;
      if (config.flags.mqtt && !Error.ConexionMqtt && strcmp("5.8.8.8", config.sensor_ip) != 0)
        publisher(config.R02_mqtt, digitalRead(PIN_RL2) ? "ON" : "OFF");
    }
    if (!digitalRead(PIN_RL3) && Flags.RelayTurnOn && config.R03Min == 999 && (config.flags.changeGridSign ? inverter.wgrid < config.R03PotOn : inverter.wgrid > config.R03PotOn))
    {
      // Start relay3
      digitalWrite(PIN_RL3, HIGH);
      INFOV("Encendido por W Salida 3\n");
      Flags.Relay03Auto = true;
      Flags.RelayTurnOn = false;
      if (config.flags.mqtt && !Error.ConexionMqtt && strcmp("5.8.8.8", config.sensor_ip) != 0)
        publisher(config.R03_mqtt, digitalRead(PIN_RL3) ? "ON" : "OFF");
    }
    if (!digitalRead(PIN_RL4) && Flags.RelayTurnOn && config.R04Min == 999 && (config.flags.changeGridSign ? inverter.wgrid < config.R04PotOn : inverter.wgrid > config.R04PotOn))
    {
      // Start relay4
      digitalWrite(PIN_RL4, HIGH);
      INFOV("Encendido por W Salida 4\n");
      Flags.Relay04Auto = true;
      Flags.RelayTurnOn = false;
      if (config.flags.mqtt && !Error.ConexionMqtt && strcmp("5.8.8.8", config.sensor_ip) != 0)
        publisher(config.R04_mqtt, digitalRead(PIN_RL4) ? "ON" : "OFF");
    }
  }

  // Stop at defined power off

  if (!(Flags.Relay04Man || config.relaysFlags.R04Man) && digitalRead(PIN_RL4) && !Flags.RelayTurnOff && config.R04Min == 999 && (config.flags.changeGridSign ? inverter.wgrid > config.R04PotOff : inverter.wgrid < config.R04PotOff))
  {
    // Start relay4
    digitalWrite(PIN_RL4, LOW);
    INFOV("Apagado por W Salida 4\n");
    Flags.Relay04Auto = false;
    Flags.RelayTurnOff = true;
    if (config.flags.mqtt && !Error.ConexionMqtt && strcmp("5.8.8.8", config.sensor_ip) != 0)
      publisher(config.R04_mqtt, digitalRead(PIN_RL4) ? "ON" : "OFF");
  }

  if (!(Flags.Relay03Man || config.relaysFlags.R03Man) && digitalRead(PIN_RL3) && !Flags.RelayTurnOff && config.R03Min == 999 && (config.flags.changeGridSign ? inverter.wgrid > config.R03PotOff : inverter.wgrid < config.R03PotOff))
  {
    // Start relay3
    digitalWrite(PIN_RL3, LOW);
    INFOV("Apagado por W Salida 3\n");
    Flags.Relay03Auto = false;
    Flags.RelayTurnOff = true;
    if (config.flags.mqtt && !Error.ConexionMqtt && strcmp("5.8.8.8", config.sensor_ip) != 0)
      publisher(config.R03_mqtt, digitalRead(PIN_RL3) ? "ON" : "OFF");
  }

  if (!(Flags.Relay02Man || config.relaysFlags.R02Man) && digitalRead(PIN_RL2) && !Flags.RelayTurnOff && config.R02Min == 999 && (config.flags.changeGridSign ? inverter.wgrid > config.R02PotOff : inverter.wgrid < config.R02PotOff))
  {
    // Start relay2
    digitalWrite(PIN_RL2, LOW);
    INFOV("Apagado por W Salida 2\n");
    Flags.Relay02Auto = false;
    Flags.RelayTurnOff = true;
    if (config.flags.mqtt && !Error.ConexionMqtt && strcmp("5.8.8.8", config.sensor_ip) != 0)
      publisher(config.R02_mqtt, digitalRead(PIN_RL2) ? "ON" : "OFF");
  }

  if (!(Flags.Relay01Man || config.relaysFlags.R01Man) && digitalRead(PIN_RL1) && !Flags.RelayTurnOff && config.R01Min == 999 && (config.flags.changeGridSign ? inverter.wgrid > config.R01PotOff : inverter.wgrid < config.R01PotOff))
  {
    // Start relay1
    digitalWrite(PIN_RL1, LOW);
    INFOV("Apagado por W Salida 1\n");
    Flags.Relay01Auto = false;
    Flags.RelayTurnOff = true;
    if (config.flags.mqtt && !Error.ConexionMqtt && strcmp("5.8.8.8", config.sensor_ip) != 0)
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

void relayManualControl(boolean forceOFF)
{

  if (config.flags.debug2) { INFOV("relayManualControl()\n"); }

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

  // Se desactivan las salidas seleccionadas manualmente si no lo están ya
  if ((!Flags.Relay01Man && !config.relaysFlags.R01Man) && digitalRead(PIN_RL1)) { digitalWrite(PIN_RL1, LOW); INFOV("Relay 1 Forced Off\n");}
  if ((!Flags.Relay02Man && !config.relaysFlags.R02Man) && digitalRead(PIN_RL2)) { digitalWrite(PIN_RL2, LOW); INFOV("Relay 2 Forced Off\n");}
  if ((!Flags.Relay03Man && !config.relaysFlags.R03Man) && digitalRead(PIN_RL3)) { digitalWrite(PIN_RL3, LOW); INFOV("Relay 3 Forced Off\n");}
  if ((!Flags.Relay04Man && !config.relaysFlags.R04Man) && digitalRead(PIN_RL4)) { digitalWrite(PIN_RL4, LOW); INFOV("Relay 4 Forced Off\n");}

  // Se fuerza el apagado de todas las salidas
  if (forceOFF)
  {
    digitalWrite(PIN_RL1, LOW); INFOV("Relay 1 Forced Off\n");
    digitalWrite(PIN_RL2, LOW); INFOV("Relay 2 Forced Off\n");
    digitalWrite(PIN_RL3, LOW); INFOV("Relay 3 Forced Off\n");
    digitalWrite(PIN_RL4, LOW); INFOV("Relay 4 Forced Off\n");
  }

  // Se fuerza el apagado de todas las salidas
  // if (forceOFF || !config.flags.pwmEnabled)
  // {
  //   if (digitalRead(PIN_RL1) && !Flags.Relay01Man && !config.relaysFlags.R01Man) { digitalWrite(PIN_RL1, LOW); INFOV("Relay 1 Forced Off\n"); }
  //   if (digitalRead(PIN_RL2) && !Flags.Relay02Man && !config.relaysFlags.R02Man) { digitalWrite(PIN_RL2, LOW); INFOV("Relay 2 Forced Off\n"); }
  //   if (digitalRead(PIN_RL3) && !Flags.Relay03Man && !config.relaysFlags.R03Man) { digitalWrite(PIN_RL3, LOW); INFOV("Relay 3 Forced Off\n"); }
  //   if (digitalRead(PIN_RL4) && !Flags.Relay04Man && !config.relaysFlags.R04Man) { digitalWrite(PIN_RL4, LOW); INFOV("Relay 4 Forced Off\n"); }
  // }
}

// PWM Functions
void shutdownPwm(boolean forceRelayOff, const char *message)
{
  if (config.flags.debug1) { INFOV(PSTR(message)); }

  myPID.SetMode(PID::MANUAL);
  PIDOutput = 0;
  Setpoint = 0;
  pwm.targetPwm = 0;
  pwm.invert_pwm = 0;
  pwm.pwmValue = 0;
  ledcWrite(2, 0); // Hard shutdown
  dac_output_disable(DAC_CHANNEL_2);
  calcPwmProgressBar();

  relayManualControl(forceRelayOff);
}

void writePwmValue(uint16_t value)
{
  if (config.flags.dimmerLowCost && value > 1023) { value -= 1023; } 
    
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

void calcPwmProgressBar()
{
  // Arreglos necesarios para mostrar el % correcto en caso de usar el dimmer low cost afectado por el fallo del 20%
  if (config.flags.dimmerLowCost) {
    if (pwm.invert_pwm > 0) { pwm.pwmValue = round(((pwm.invert_pwm - 210) * 100.0) / (config.maxPwmLowCost - 210.0)); }
    else pwm.pwmValue = 0;
  } else {  
    pwm.pwmValue = round((pwm.invert_pwm * 100.0) / 1023.0);
  }
}