# Software Design Document — Firmware ESP32 WildTrack

## 1. Objetivo del firmware

El firmware del ESP32 será responsable de operar un comedero inteligente de fauna silvestre, capturar datos de sensores, construir eventos locales y publicarlos mediante MQTT.

El ESP32 no accede directamente a PostgreSQL ni MongoDB. Su única integración externa será mediante MQTT hacia el broker.

El identificador principal del dispositivo será `device_id`, generado previamente por la plataforma backend. La MAC solo se usará para diagnóstico.

---

## 2. Principios de diseño

El firmware debe cumplir estos principios:

1. Modularidad: cada módulo debe tener una única responsabilidad.
2. Bajo acoplamiento: sensores, MQTT, almacenamiento y lógica de dominio no deben mezclarse.
3. Implementación incremental: cada paso debe poder probarse por consola antes de integrarse.
4. Tolerancia a fallos: el dispositivo debe poder recuperarse ante pérdida de WiFi, MQTT o errores de sensores.
5. Eventos únicos y completos: una sesión de alimentación debe producir un único evento final.
6. Persistencia mínima: configuración crítica guardada en flash.
7. Extensibilidad: debe permitir agregar sensores ambientales o nuevos eventos sin reescribir todo.

---

## 3. Arquitectura general del firmware

El firmware se dividirá en capas:

### 3.1 Drivers de hardware

Responsables de hablar directamente con sensores y actuadores.

Módulos:

* `DistanceSensor`
* `CameraService`
* `RfidReader`
* `ScaleSensor`
* `EnvironmentSensor`
* `DispenserMotor`
* `RotationSensor`
* `LimitSwitch`
* `StatusLed`

Estos módulos no conocen MQTT, JSON ni reglas de negocio.

**Política de cámara:** `CameraService` forma parte del alcance del proyecto y ha sido validada físicamente. Sin embargo, nunca debe bloquear una sesión de alimentación. Si la captura falla, la sesión continúa normalmente y el evento MQTT marcará el error correspondiente. El backend decidirá cómo manejar esa condición.

---

### 3.2 Servicios de dominio

Responsables de interpretar lecturas y coordinar el comportamiento del comedero.

Módulos:

* `PresenceService`
* `FeedingSessionService`
* `DispenserService`
* `FoodLevelService`
* `DeviceStateMachine`

Estos módulos responden preguntas como:

* ¿hay un animal presente?
* ¿comenzó una sesión de alimentación?
* ¿terminó una sesión?
* ¿cuánto alimento se consumió?
* ¿debe activarse el motor del dispensador?
* ¿hay alimento en el depósito?

---

### 3.3 Comunicación

Responsables de conectividad externa.

Módulos:

* `WifiService`
* `MqttService`
* `MqttTopics`
* `MqttPublisher`

Estos módulos no leen sensores directamente. Solo publican mensajes ya construidos.

---

### 3.4 Telemetría

Responsable de construir payloads.

Módulos:

* `TelemetryEvent`
* `TelemetryBuilder`
* `JsonSerializer`

Tipos de eventos iniciales:

* `feeding_session`
* `heartbeat`
* `device_boot`
* `hardware_error`
* `low_food_level`
* `dispenser_refill`
* `empty_reservoir`

---

### 3.5 Configuración y almacenamiento

Responsables de identidad, parámetros y persistencia local.

Módulos:

* `DeviceConfig`
* `ConfigStorage`
* `ProvisioningService`
* `EventQueueStorage`

Para el MVP se guardará en flash:

* `device_id`
* `serial_number`
* `wifi_ssid`
* `wifi_password`
* `mqtt_host`
* `mqtt_port`
* `firmware_version`
* parámetros de calibración del HX711

---

### 3.6 Servicios de sistema

Responsables de exponer información del hardware y del estado del dispositivo al resto del firmware. El resto de los módulos no debe acceder directamente a las APIs del ESP32 para obtener esta información.

Módulos:

* `DeviceInfoService`

**Responsabilidad de `DeviceInfoService`:**

Encapsular toda la información del hardware y estado del ESP32:

* MAC Address
* Chip Model
* Chip Revision
* Flash Size (total)
* PSRAM disponible
* Firmware Version
* Uptime (ms)
* RSSI (solo cuando hay conexión WiFi activa; devuelve 0 si no hay conexión)

Este módulo vive en `src/diagnostics/` y es el único punto de acceso a estas APIs. La telemetría (heartbeat, device_boot) y el provisioning consumen este servicio.

---

## 4. Organización de carpetas

```txt
firmware/
├── platformio.ini
├── README.md
├── docs/
│   └── SDD.md
├── include/
│   └── WildTrack.h
├── src/
│   ├── main.cpp
│   ├── config/
│   │   ├── Pins.h
│   │   ├── FirmwareConfig.h
│   │   ├── DeviceConfig.h
│   │   └── MqttTopics.h
│   ├── core/
│   │   ├── DeviceStateMachine.h
│   │   ├── DeviceStateMachine.cpp
│   │   ├── TimeService.h
│   │   └── TimeService.cpp
│   ├── storage/
│   │   ├── ConfigStorage.h
│   │   ├── ConfigStorage.cpp
│   │   ├── EventQueueStorage.h
│   │   └── EventQueueStorage.cpp
│   ├── provisioning/
│   │   ├── ProvisioningService.h
│   │   └── ProvisioningService.cpp
│   ├── wifi/
│   │   ├── WifiService.h
│   │   └── WifiService.cpp
│   ├── mqtt/
│   │   ├── MqttService.h
│   │   ├── MqttService.cpp
│   │   ├── MqttPublisher.h
│   │   └── MqttPublisher.cpp
│   ├── sensors/
│   │   ├── DistanceSensor.h
│   │   ├── DistanceSensor.cpp
│   │   ├── EnvironmentSensor.h
│   │   └── EnvironmentSensor.cpp
│   ├── scale/
│   │   ├── ScaleSensor.h
│   │   ├── ScaleSensor.cpp
│   │   ├── ScaleCalibrator.h
│   │   └── ScaleCalibrator.cpp
│   ├── rfid/
│   │   ├── RfidReader.h
│   │   └── RfidReader.cpp
│   ├── camera/
│   │   ├── CameraService.h
│   │   └── CameraService.cpp
│   ├── dispenser/
│   │   ├── DispenserService.h
│   │   ├── DispenserService.cpp
│   │   ├── DispenserMotor.h
│   │   ├── DispenserMotor.cpp
│   │   ├── RotationSensor.h
│   │   ├── RotationSensor.cpp
│   │   ├── LimitSwitch.h
│   │   └── LimitSwitch.cpp
│   ├── telemetry/
│   │   ├── TelemetryEvent.h
│   │   ├── TelemetryBuilder.h
│   │   ├── TelemetryBuilder.cpp
│   │   ├── JsonSerializer.h
│   │   └── JsonSerializer.cpp
│   └── diagnostics/
│       ├── Logger.h
│       ├── Logger.cpp
│       ├── StatusLed.h
│       ├── StatusLed.cpp
│       ├── DeviceInfoService.h
│       └── DeviceInfoService.cpp
└── test/
    └── README.md
```

---

## 5. Especificación de hardware y conexiones

### Placa principal

**ESP32-S3-CAM N16R8**
- Flash: 16 MB QIO
- PSRAM: 8 MB OPI (usado para frame buffer de cámara)
- LED RGB integrado: GPIO 48

---

### Cámara — OV2640

Módulo con conector FFC estilo ESP32S3-EYE. GPIOs 4–18 reservados exclusivamente para cámara.

| Señal  | GPIO | Notas                        |
|--------|------|------------------------------|
| XCLK   | 15   | Clock de salida al sensor    |
| SIOD   | 4    | I²C SDA (SCCB)               |
| SIOC   | 5    | I²C SCL (SCCB)               |
| Y2     | 11   | Datos píxel bit 0            |
| Y3     | 9    | Datos píxel bit 1            |
| Y4     | 8    | Datos píxel bit 2            |
| Y5     | 10   | Datos píxel bit 3            |
| Y6     | 12   | Datos píxel bit 4            |
| Y7     | 18   | Datos píxel bit 5            |
| Y8     | 17   | Datos píxel bit 6            |
| Y9     | 16   | Datos píxel bit 7            |
| VSYNC  | 6    | Sincronización vertical      |
| HREF   | 7    | Sincronización horizontal    |
| PCLK   | 13   | Pixel clock                  |
| VCC    | 3.3V |                              |
| GND    | GND  |                              |

Configuración firmware: `FRAMESIZE_QVGA` (320×240), JPEG calidad 12, frame en PSRAM, ~10 KB por foto.

> ⚠️ Desconectar VCC de la cámara antes de subir firmware — el consumo de corriente impide que el chip entre en modo de descarga.

---

### Sensor de distancia — GP2Y0A21YK0F (Sharp IR)

Sensor analógico infrarrojo, rango 10–80 cm.

| Pin sensor | Conexión ESP32 |
|------------|----------------|
| VCC        | 5V             |
| GND        | GND            |
| Vout       | GPIO 14 (ADC)  |

Configuración firmware: `ADC_11db`, promedio de 20 lecturas, fórmula `d = 29.988 × V^(−1.173)`. Umbral de presencia: 10 cm.

---

### Báscula — HX711 + celda de carga

| Pin HX711 | Conexión ESP32 |
|-----------|----------------|
| VCC       | 3.3V           |
| GND       | GND            |
| DOUT      | GPIO 3         |
| SCK       | GPIO 2         |

Celda de carga conectada a E+/E−/A+/A− del HX711. Factor de calibración y offset de tare guardados en flash.

---

### Sensor ambiental — DHT22

| Pin DHT22 | Conexión ESP32       |
|-----------|----------------------|
| VCC       | 3.3V                 |
| GND       | GND                  |
| DATA      | GPIO 21              |
| (DATA)    | 10 kΩ pullup a 3.3V  |

Mide temperatura (°C) y humedad relativa (%).

---

### Lector RFID

Módulo UART, solo RX desde el ESP32 (el lector transmite el tag al ESP32).

| Pin RFID | Conexión ESP32 |
|----------|----------------|
| VCC      | 5V             |
| GND      | GND            |
| TX       | GPIO 42 (RX2)  |

---

### Sistema de dispensación

#### Motor — FT-49OGM500-530K

Motorreductor DC, 3.7 V, 7.5 RPM en eje de salida.

#### Driver puente H — MX1616

| Pin MX1616 | Conexión ESP32 | Función                    |
|------------|----------------|----------------------------|
| IN1        | GPIO 38        | Dirección FORWARD          |
| IN2        | GPIO 39        | Dirección REVERSE          |
| OUT1/OUT2  | Motor +/−      | Salida al motor            |
| VCC lógica | 3.3V           |                            |
| VM motor   | 3.7V (batería) |                            |
| GND        | GND común      |                            |

Control firmware: `IN1=HIGH, IN2=LOW` → FORWARD / `IN1=LOW, IN2=HIGH` → REVERSE / ambos LOW → freno.

#### Sensor óptico de rotación — SPD_DET_V1.0

Montado sobre el **eje pre-reductor** del motor (antes de la caja reductora). Disco ranurado interrumpe el haz IR.

| Componente         | Conexión                              |
|--------------------|---------------------------------------|
| Q1 IR LED Rojo (A) | 220 Ω → 3.3V                         |
| Q1 IR LED Negro (K)| GND                                   |
| Q2 fototransistor Blanco (C) | 10 kΩ → 3.3V **y** GPIO 41 |
| Q2 fototransistor Amarillo (E) | GND                       |

Señal: digital, pulso bajo cuando el disco interrumpe el haz.
Velocidad: ~72 pulsos/segundo en REVERSE a 3.7 V. Máximo mecánico: ~100 pulsos por fase.

> Nota: en dirección FORWARD el sensor genera interferencia eléctrica (~6000 pulsos/s de ruido). Por eso FORWARD se controla por tiempo fijo (2500 ms) y REVERSE por conteo de pulsos.

#### Limit switch — fin de carrera

Detecta si hay alimento en el depósito. Tipo NO (normalmente abierto).

| Pin switch | Conexión ESP32                 |
|------------|--------------------------------|
| Terminal 1 | GPIO 40 (INPUT_PULLUP)        |
| Terminal 2 | GND                            |

Lógica: `LOW` = alimento disponible (switch cerrado). `HIGH` = depósito vacío.

---

### LED RGB integrado

GPIO 48, tipo WS2812 (NeoPixel), 1 LED.

---

### Resumen de pines

| GPIO | Función               | Dirección |
|------|-----------------------|-----------|
| 2    | HX711 SCK             | Output    |
| 3    | HX711 DOUT            | Input     |
| 4    | Cámara SIOD           | I²C       |
| 5    | Cámara SIOC           | I²C       |
| 6    | Cámara VSYNC          | Input     |
| 7    | Cámara HREF           | Input     |
| 8    | Cámara Y4             | Input     |
| 9    | Cámara Y3             | Input     |
| 10   | Cámara Y5             | Input     |
| 11   | Cámara Y2             | Input     |
| 12   | Cámara Y6             | Input     |
| 13   | Cámara PCLK           | Input     |
| 14   | Sensor distancia Vout | ADC Input |
| 15   | Cámara XCLK           | Output    |
| 16   | Cámara Y9             | Input     |
| 17   | Cámara Y8             | Input     |
| 18   | Cámara Y7             | Input     |
| 21   | DHT22 DATA            | Input     |
| 38   | Motor IN1 (FORWARD)   | Output    |
| 39   | Motor IN2 (REVERSE)   | Output    |
| 40   | Limit switch          | Input     |
| 41   | Rotation sensor DO    | Input     |
| 42   | RFID RX (Serial2)     | Input     |
| 48   | RGB LED               | Output    |

---

## 6. Máquina de estados del dispositivo

Estados principales:

```txt
BOOTING
↓
LOAD_CONFIG
↓
PROVISIONING_REQUIRED / WIFI_CONNECTING
↓
MQTT_CONNECTING
↓
IDLE
↓
PRESENCE_DETECTED
↓
FEEDING_SESSION_ACTIVE
↓
FEEDING_SESSION_CLOSING
↓
PUBLISHING_EVENT
↓
IDLE
```

Estados de error:

```txt
WIFI_DISCONNECTED
MQTT_DISCONNECTED
SENSOR_ERROR
CAMERA_ERROR
STORAGE_ERROR
DISPENSER_ERROR
```

El dispositivo debe intentar recuperarse automáticamente cuando sea posible.

---

## 7. Flujo de sesión de alimentación

### Inicio

Una sesión inicia cuando el sensor de proximidad detecta presencia a menos de 10 cm durante un tiempo mínimo estable.

Esto evita falsos positivos por ruido.

### Durante la sesión

El firmware debe:

1. Crear un `session_id` local.
2. Capturar timestamp de inicio.
3. Capturar 2 a 4 fotografías.
4. Leer peso inicial.
5. Intentar leer RFID durante una ventana de tiempo definida.
6. Leer temperatura y humedad.
7. Mantener la sesión abierta mientras exista presencia.

### Cierre

Cuando el sensor de proximidad indica que el animal se retiró:

1. Confirmar ausencia durante un tiempo mínimo.
2. Leer peso final.
3. Calcular gramos consumidos.
4. Construir evento `feeding_session`.
5. Publicar evento MQTT.
6. Volver a estado `IDLE`.

---

## 8. Evento principal MQTT

Tipo: `feeding_session`

Payload conceptual:

```json
{
  "event_type": "feeding_session",
  "device_id": "uuid-v7",
  "session_id": "local-session-id",
  "timestamp_start": "ISO-8601",
  "timestamp_end": "ISO-8601",
  "rfid_tag": "optional",
  "weight_initial_g": 1250.5,
  "weight_final_g": 1198.2,
  "food_consumed_g": 52.3,
  "temperature_c": 24.8,
  "humidity_percent": 61.4,
  "wifi_rssi": -63,
  "battery_percent": null,
  "dispenser_state": "closed",
  "photos": [
    {
      "photo_id": "local-photo-1",
      "capture_offset_ms": 0
    }
  ],
  "firmware_version": "0.1.0"
}
```

Para el MVP, las fotos pueden omitirse del payload o representarse como metadatos si todavía no existe almacenamiento/subida de imágenes.

---

## 9. Estrategia MQTT

### Topics propuestos

Publicación de eventos:

```txt
wildtrack/devices/{device_id}/events
```

Heartbeat:

```txt
wildtrack/devices/{device_id}/heartbeat
```

Estado del dispositivo:

```txt
wildtrack/devices/{device_id}/status
```

Errores:

```txt
wildtrack/devices/{device_id}/errors
```

Comandos futuros:

```txt
wildtrack/devices/{device_id}/commands
```

Respuesta a comandos:

```txt
wildtrack/devices/{device_id}/commands/response
```

### QoS

Para MVP:

```txt
feeding_session: QoS 1
heartbeat: QoS 0
errors: QoS 1
status: QoS 0
```

### Last Will

El dispositivo debe registrar un Last Will en:

```txt
wildtrack/devices/{device_id}/status
```

Payload:

```json
{
  "status": "offline",
  "reason": "unexpected_disconnect"
}
```

Cuando conecte correctamente, publicará:

```json
{
  "status": "online"
}
```

---

## 10. Provisioning

### MVP

Para el MVP, el provisionamiento será manual/semi-manual:

1. Se crea el dispositivo en la plataforma.
2. El backend genera `device_id`.
3. El usuario carga en el firmware:

   * `device_id`
   * WiFi SSID
   * WiFi password
   * MQTT host
   * MQTT port
4. El ESP32 guarda esa configuración en flash.
5. El ESP32 arranca usando esa configuración.

Este enfoque es suficiente para desarrollo académico y evita complejidad inicial.

### Producción

Para producción, se recomienda:

1. ESP32 arranca en modo Access Point si no tiene configuración.
2. El usuario se conecta desde celular o web local.
3. Ingresa credenciales WiFi y un token de provisionamiento.
4. El ESP32 consulta la plataforma.
5. La plataforma devuelve `device_id` y configuración MQTT.
6. El ESP32 guarda todo en flash.
7. Reinicia en modo operativo.

---

## 11. Almacenamiento en flash

Se usará almacenamiento tipo key-value para configuración persistente.

Datos persistentes:

```txt
device_id
serial_number
wifi_ssid
wifi_password
mqtt_host
mqtt_port
firmware_version
hx711_calibration_factor
hx711_tare_offset
```

Más adelante se puede agregar cola offline de eventos.

Para MVP no es obligatorio almacenar eventos no publicados, pero sí recomendable en una fase posterior.

---

## 12. Recuperación ante errores

### WiFi perdido

* Reintentar conexión.
* No bloquear lectura de sensores.
* Marcar estado `WIFI_DISCONNECTED`.
* Si ocurre una sesión durante desconexión, guardarla localmente si existe cola offline.

### MQTT perdido

* Mantener WiFi.
* Intentar reconexión con backoff.
* No perder sesión activa.
* Publicar eventos pendientes cuando MQTT vuelva.

### Sensor falla

* Registrar error.
* Publicar evento `hardware_error` si MQTT está disponible.
* Continuar operación si el sensor no es crítico.

### Cámara falla

* La cámara forma parte del alcance del proyecto y ha sido validada físicamente.
* Si la captura falla, la sesión **continúa normalmente** — la cámara nunca bloquea una sesión.
* El evento MQTT debe marcar `camera_error: true`.
* El backend decide cómo manejar la ausencia de fotos.

### HX711 falla

* La sesión queda incompleta.
* El evento debe marcar `weight_status: unavailable`.

### Dispenser falla

* Si el motor está energizado pero el `RotationSensor` no genera pulsos, detectar atasco.
* Si el tiempo máximo de dispensación es excedido sin completar los pulsos esperados, detener el motor inmediatamente.
* Registrar `DISPENSER_ERROR`.
* No reintentar la dispensación automáticamente para evitar daños al mecanismo.

---

## 13. Sistema de dispensación

El dispensador utiliza un motorreductor DC (FT-49OGM500-530K, 3.7 V, 7.5 RPM) controlado mediante el driver puente H MX1616, y un sensor óptico de rotación (SPD_DET_V1.0) montado sobre el eje pre-reductor del motor.

### Mecanismo bidireccional

El comedero tiene dos aletas: una activa cuando el motor gira hacia adelante (FORWARD) y otra cuando gira hacia atrás (REVERSE). Ambas deben activarse en cada dispensación para prevenir atascos y dosificar en dos direcciones.

### Principios de control

* **FORWARD** se controla por **tiempo fijo** (`FORWARD_RUN_MS = 2500 ms`). El sensor óptico genera señal espuria en este sentido (interferencia eléctrica del motor), por lo que no es confiable para conteo.
* **REVERSE** se controla por **conteo de pulsos** del `RotationSensor`. El sensor óptico genera pulsos reales en esta dirección.
* El parámetro `pulsesPerSide` del comando `dispense` controla únicamente la fase REVERSE.
* El encoder está en el eje pre-reductor y genera aproximadamente 72 pulsos/segundo en REVERSE a 5 V. El máximo mecánico es ~100 pulsos por fase.
* Cada ciclo completo es: FORWARD (2500 ms) → pausa 150 ms → REVERSE (N pulsos) → pausa 150 ms → siguiente ciclo.

### Detección de atascos

* Si el motor corre en REVERSE y el `RotationSensor` no alcanza los pulsos esperados dentro de `DIR_TIMEOUT_MS` (35 s), se considera atasco.
* El motor se detiene inmediatamente.
* Se registra `DISPENSER_ERROR`.
* FORWARD no tiene detección de atasco (control por tiempo).

### Verificación de alimento

* El `LimitSwitch` en GPIO 6 detecta si existe alimento suficiente en el depósito.
* Si el `LimitSwitch` indica depósito vacío, el motor no debe activarse.
* Se publica evento `empty_reservoir`.

### Responsabilidades por módulo

* `DispenserMotor`: driver de bajo nivel. Enciende y apaga el motor via MX1616. No conoce pulsos ni lógica de negocio.
* `RotationSensor`: driver de bajo nivel. Cuenta pulsos del sensor óptico en el eje pre-reductor. No conoce el motor ni la sesión.
* `LimitSwitch`: driver de bajo nivel. Lee el estado del interruptor de depósito.
* `DispenserService`: servicio de dominio. Coordina `DispenserMotor`, `RotationSensor` y `LimitSwitch`. FORWARD por tiempo, REVERSE por pulsos, timeout y error.

---

## 14. Estrategia de pruebas

Cada paso debe verificarse antes de avanzar.

Tipos de prueba:

### Prueba por consola serial

Validar lectura o comportamiento aislado.

### Prueba de módulo

Validar un driver sin MQTT ni lógica global.

### Prueba de integración local

Validar interacción entre sensores y servicios.

### Prueba de integración MQTT

Validar publicación real hacia broker.

### Prueba de sesión completa

Simular llegada y retiro de animal.

### Prueba de recuperación

Desconectar WiFi, broker o sensor y validar que el firmware no se cuelgue.

---

## 15. Roadmap incremental

### Firmware Step 0 — Proyecto base ✅ COMPLETADO

Entregables validados en hardware:
* PlatformIO configurado para ESP32-S3-CAM N16R8.
* Firmware compila y carga correctamente.
* Serial Monitor muestra nombre, versión y MAC.
* Estructura de carpetas alineada con el SDD.

---

### Firmware Step 1 — Identidad del dispositivo

Objetivo: Crear `DeviceInfoService` como única fuente de verdad del hardware del ESP32. Exponer MAC Address, chip info, flash, PSRAM, uptime y firmware version. Derivar `serial_number` temporal desde la MAC.

Verificación:
* Serial muestra MAC, chip model, chip revision, flash size, PSRAM.
* Serial muestra `serial_number` derivado (formato `WT-XXYYZZ`).
* Serial muestra firmware version.
* Ningún dato de hardware se lee fuera de `DeviceInfoService`.

---

### Firmware Step 2 — Configuración local

Objetivo: Definir `DeviceConfig` y almacenamiento en flash.

---

### Firmware Step 3 — Provisioning MVP

Objetivo: Cargar manualmente `device_id`, WiFi y MQTT config.

---

### Firmware Step 4 — WiFi

Objetivo: Conectar a WiFi de forma modular.

---

### Firmware Step 5 — MQTT base

Objetivo: Conectar al broker MQTT.

---

### Firmware Step 6 — Sensor de proximidad

Objetivo: Integrar Sharp como `DistanceSensor` y `PresenceService`.

---

### Firmware Step 7 — DHT22

Objetivo: Integrar temperatura y humedad.

---

### Firmware Step 8 — HX711 calibrado

Objetivo: Integrar celda de carga con lectura en gramos.

---

### Firmware Step 9 — RFID

Objetivo: Integrar lector RFID.

---

### Firmware Step 10 — Sistema de dispensación ✅ COMPLETADO

Hardware:
* Motorreductor DC FT-49OGM500-530K (3.7 V, operado a 5 V)
* Driver puente H MX1616 (IN1=GPIO38, IN2=GPIO39)
* Sensor óptico de rotación SPD_DET_V1.0 (eje pre-reductor, GPIO10)
  * Q1 (IR LED): Rojo(A) → 220Ω → 3.3V, Negro(K) → GND
  * Q2 (fototransistor): Blanco(C) → 10kΩ → 3.3V y GPIO10, Amarillo(E) → GND
* LimitSwitch NO (GPIO6, INPUT_PULLUP, LOW=alimento disponible)

Flujo por ciclo:

```txt
Verificar alimento (LimitSwitch)
↓
FORWARD 2500 ms (aleta A — control por tiempo)
↓
Pausa 150 ms
↓
REVERSE hasta N pulsos ópticos (aleta B — control por pulsos)
├─ Pulsos alcanzados → pausa 150 ms → siguiente ciclo o IDLE
└─ Timeout 35 s sin pulsos → DISPENSER_ERROR
```

Notas de implementación:
* FORWARD usa tiempo fijo: el sensor genera interferencia eléctrica en este sentido.
* REVERSE usa pulsos reales del encoder óptico (~72 pulsos/s a 5 V, máx ~100 pulsos por fase).
* Comando `dispense [ciclos] [pulsos]`: `pulsos` controla la fase REVERSE.
* Comando disponible via `ProvisioningService` por Serial.

Criterios de aceptación validados:
* Ambas aletas se mueven visiblemente en cada ciclo.
* El `RotationSensor` genera pulsos confiables en sentido REVERSE.
* El `LimitSwitch` detecta depósito vacío e impide activación.
* Un atasco en REVERSE genera `DISPENSER_ERROR` y detiene el motor.

---

### Firmware Step 11 — Cámara ✅ COMPLETADO

Objetivo: Capturar fotografías asociadas a una sesión.

Implementación: `CameraService` en `src/camera/`. OV2640, QVGA JPEG (~10 KB) en PSRAM.
Pines migrados para liberar GPIOs de cámara: `PIN_DISTANCE_SENSOR=14`, `PIN_DHT22=21`, `PIN_LIMIT_SWITCH=40`, `PIN_ROTATION_SENSOR=41`, `PIN_RFID_RX=42`.

---

### Firmware Step 12 — Evento local de sesión ✅ COMPLETADO

Objetivo: Construir un evento `feeding_session` sin publicarlo aún.

Implementación: `FeedingSessionService` en `src/core/`.

Flujo:
* Presencia estable ≥1500 ms → inicia sesión: genera event_id (UUID v4 via `esp_fill_random`), lee peso inicial, captura foto, lee temp/hum.
* Durante sesión: captura RFID si llega, monitorea ausencia.
* Ausencia confirmada ≥3000 ms → cierra sesión: lee peso final, timestamp ISO 8601 (fin de sesión), construye JSON.
* `hasEvent()=true` bloquea nuevas sesiones hasta que Step 13 publique y llame `clearEvent()`.

JSON construido (contrato backend Slice 6):
```json
{
  "event_id": "uuid-v4",
  "event_type": "feeding_session",
  "device_id": "uuid",
  "timestamp": "ISO-8601-end",
  "rfid": { "detected": false, "tag": null, "read_quality": null },
  "sensors": { "temperature_c": 31.5, "humidity_pct": 40.5,
                "initial_weight_g": 146.5, "final_weight_g": 0.0, "consumed_g": 146.5 },
  "media": { "captured": 1, "urls": [] },
  "device_status": { "wifi_rssi_dbm": -73, "firmware_version": "0.1.0", "battery_pct": null }
}
```

Notas:
* `consumed_g = max(initial - final, 0)`.
* `timestamp` = hora de cierre de sesión (un solo campo, sin station_id).
* El frame JPEG queda en PSRAM hasta que Step 13 lo libere via `clearEvent()`.

---

### Firmware Step 13 — Publicación MQTT de sesión ✅ COMPLETADO

Objetivo: Publicar evento `feeding_session` con foto.

Flujo:
* `hasEvent()` → HTTP POST JPEG a `POST /api/v1/media/upload?device_id=&event_id=` (puerto 8000).
* Si upload OK → `setPhotoUrl(url)` reconstruye JSON con `urls: ["<url>"]`.
* Publica JSON via MQTT. Si OK → `clearEvent()` libera PSRAM y habilita nueva sesión.
* Si upload falla → publica igual con `urls: []`, sin bloquear la sesión.

Implementación: `MediaUploadService` en `src/media/`. Buffer MQTT ampliado a 1024 bytes.

---

### Firmware Step 14 — Heartbeat

Objetivo: Publicar estado periódico del dispositivo.

---

### Firmware Step 15 — Eventos secundarios

Objetivo: Agregar eventos de error, depósito vacío y recarga.

---

### Firmware Step 16 — Cola offline

Objetivo: Guardar eventos si MQTT no está disponible.

---

## 16. Decisiones importantes

1. MQTT se implementa antes de todos los sensores, pero después del provisioning.
2. La cámara se deja después de sensores críticos porque es más inestable y pesada.
3. El evento principal se construye primero localmente y luego se publica.
4. El sistema de dispensación se integra antes de la sesión completa, pero nunca debe bloquear la lógica de detección ni la comunicación MQTT.
5. La cola offline se deja para una fase posterior al MVP.
6. La lógica de sesión debe vivir en `FeedingSessionService`, no en `main.cpp`.

---

## 17. Alcance recomendado para MVP

El MVP realista debería incluir:

* Configuración local.
* WiFi.
* MQTT.
* Proximidad.
* DHT22.
* HX711 calibrado.
* RFID.
* Evento `feeding_session`.
* Publicación MQTT.
* Heartbeat básico.

La cámara, servo automático y cola offline pueden entrar después si el tiempo se complica.
