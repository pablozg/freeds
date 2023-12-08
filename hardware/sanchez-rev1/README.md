
La arquitectura "Sánchez rev1" se basa en las ideas de otras arquitecturas hardware para FreeDS, separando el diseño en dos PCBs que caben en las cajas estancas usadas por otras arquitecturas hardware.

La arquitectura Sánchez rev1 incluye:

- Conectores para RS485
- Conectores para sensores de temperatura
  - Espacio para un sensor montado en la parte inferior de la placa de control, pensado para que doblando las patillas pueda tocar el disipador del TRIAC
- Conectores para relés
  - Conector para ventilador controlado por relé 4
- Conectores para pinza amperimétrica
  - Conector de dos pines
  - Conector de jack de audio de 3.5mm

La novedad con respecto a otras arquitecturas es la circuitería y conectores para poder controlar un ventilador por PWM: la idea es que cuanto mayor sea el porcentaje de PWM, más trabajará el ventilador.

El diseño está hecho con KiCAD, y se incluyen los ficheros Gerber (ya compilados y por separado para cada una de las dos PCBs) para comodidad a la hora de pedir la fabricación de las placas.
