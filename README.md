# WildTrack Firmware

Firmware para comedero inteligente de fauna silvestre. Desarrollado en C++ con PlatformIO + Arduino para **ESP32-S3-CAM N16R8** (16 MB Flash, 8 MB PSRAM).

---

## Requisitos

- PlatformIO (VS Code o CLI)
- ESP32-S3-CAM N16R8
- Puerto serial a 115200 baudios

---

## Compilar y subir

```bash
pio run --target upload
```

> Si la cámara está conectada, desconectar su VCC antes de subir firmware y reconectarla después. La cámara consume suficiente corriente para impedir que el chip entre en modo de descarga.

---

## Consola serial

Toda la configuración y los comandos de diagnóstico se ejecutan por el **Serial Monitor a 115200 baudios**. Los comandos no distinguen mayúsculas/minúsculas.

---

### Provisioning — configuración inicial

El dispositivo arranca en modo **PROVISIONING_REQUIRED** si no tiene configuración guardada en flash. En ese estado solo acepta comandos `set` y `save`.

| Comando | Descripción |
|---|---|
| `set device_id <uuid>` | UUID del dispositivo (generado por el backend) |
| `set wifi_ssid <nombre>` | Nombre de la red WiFi |
| `set wifi_pass <password>` | Contraseña WiFi |
| `set mqtt_host <ip_o_host>` | IP o hostname del broker MQTT |
| `set mqtt_port <puerto>` | Puerto del broker (ej. `1883`) |
| `set mqtt_user <usuario>` | Usuario MQTT (si el broker requiere autenticación) |
| `set mqtt_pass <password>` | Contraseña MQTT |
| `status` | Muestra la configuración actual en memoria |
| `save` | Valida, guarda en flash y reinicia el dispositivo |
| `clear` | Borra toda la configuración de flash y reinicia |

**Ejemplo de provisioning completo:**
```
set device_id 06a3da4c-4759-706c-8000-7b86ae7eb4ca
set wifi_ssid MiRed
set wifi_pass MiPassword123
set mqtt_host 192.168.1.100
set mqtt_port 1883
set mqtt_user wildtrack_device
set mqtt_pass secreto
save
```

> `save` verifica que `device_id`, `wifi_ssid`, `wifi_pass`, `mqtt_host` y `mqtt_port` estén definidos antes de guardar. `mqtt_user` y `mqtt_pass` son opcionales (si el broker acepta conexiones anónimas).

---

### Báscula — calibración HX711

La celda de carga requiere dos pasos: tare (cero) y calibración con peso conocido. Ambos valores se guardan automáticamente en flash.

| Comando | Descripción |
|---|---|
| `tare` | Registra el cero con la plataforma vacía (promedia 10 lecturas). Guarda el offset en flash. |
| `cal <gramos>` | Calibra con un peso conocido colocado sobre la plataforma. Calcula y guarda el factor de escala. |

**Procedimiento de calibración:**
```
# 1. Con la plataforma completamente vacía:
tare

# 2. Colocar un peso conocido (ej. 200g) sobre la plataforma:
cal 200

# El factor queda guardado. El dispositivo ya no necesita recalibrarse al reiniciar.
```

> La calibración es válida mientras no se cambie la celda de carga ni su montaje físico. Si se reubica la celda, repetir el procedimiento.

---

### Dispensador

Activa el motor manualmente para probar el mecanismo de dispensación. El dispensador ejecuta ciclos bidireccionales: cada ciclo mueve el motor hacia adelante (tiempo fijo 2500 ms) y luego hacia atrás (conteo de pulsos del encoder óptico).

| Comando | Descripción |
|---|---|
| `dispense` | Dispensa con valores por defecto: 3 ciclos, 5 pulsos/lado |
| `dispense <ciclos>` | Dispensa N ciclos con 5 pulsos/lado |
| `dispense <ciclos> <pulsos>` | Dispensa N ciclos con M pulsos/lado (máx. 100 pulsos) |

**Ejemplos:**
```
dispense          # 3 ciclos, 5 pulsos/lado
dispense 1 10     # 1 ciclo, 10 pulsos/lado (~0.14 s en REVERSE)
dispense 5 80     # 5 ciclos, 80 pulsos/lado
```

> Límites: ciclos 1–20, pulsos 1–100. Si el encoder no detecta los pulsos esperados en 35 s, el motor se detiene por seguridad (condición de atasco).

---

### Cámara

Captura una foto de prueba y reporta sus dimensiones y tamaño. No guarda ni publica la imagen.

| Comando | Descripción |
|---|---|
| `photo` | Captura un frame JPEG (320×240, PSRAM) y muestra su tamaño |

**Ejemplo de salida:**
```
[OK] Foto: 320x240 | 9.9 KB
```

---

### Diagnóstico general

| Comando | Descripción |
|---|---|
| `status` | Muestra toda la configuración almacenada en memoria |
| `help` | Lista todos los comandos disponibles |

**Salida de `status`:**
```
--- Config en memoria ---
  device_id:  06a3da4c-4759-706c-8000-7b86ae7eb4ca
  wifi_ssid:  MiRed
  wifi_pass:  ****
  mqtt_host:  192.168.1.100
  mqtt_port:  1883
  mqtt_user:  wildtrack_device
  mqtt_pass:  ****
-------------------------
```

---

## Log en ejecución

Una vez provisionado y conectado, el firmware imprime una línea de estado cada 5 segundos:

```
[45s] dist=12.3cm pres=SI | peso=450.2g temp=29.1C hum=40.5% | MQTT=OK | rot=0 enc=1
```

| Campo | Descripción |
|---|---|
| `dist` | Distancia medida por el sensor IR (cm) |
| `pres` | Presencia detectada (SI/NO) |
| `peso` | Peso actual en la plataforma (g) |
| `temp` / `hum` | Temperatura (°C) y humedad relativa (%) |
| `MQTT` | Estado de la conexión MQTT (OK / off) |
| `rot` | Contador de pulsos del encoder de rotación |
| `enc` | Nivel digital actual del encoder (0/1) |

---

## Tópicos MQTT

| Tópico | QoS | Descripción |
|---|---|---|
| `wildtrack/devices/{device_id}/events` | 1 | Evento de sesión de alimentación (JSON) |
| `wildtrack/devices/{device_id}/telemetry` | 0 | Telemetría periódica cada 60 s |
| `wildtrack/devices/{device_id}/status` | 0 | Last Will: `{"status":"offline","reason":"unexpected_disconnect"}` |

---

## Flujo de una sesión de alimentación

```
Animal presente ≥ 1500 ms
        ↓
Inicio de sesión: genera event_id (UUID v4), lee peso inicial,
                  captura foto (PSRAM), lee temperatura/humedad
        ↓
Durante sesión: captura RFID si llega
        ↓
Animal ausente ≥ 3000 ms
        ↓
Cierre de sesión: lee peso final, obtiene timestamp ISO 8601 (UTC)
        ↓
Sube foto → POST /api/v1/media/upload → obtiene URL
        ↓
Publica evento JSON → MQTT topic /events
        ↓
clearEvent() → libera PSRAM → acepta nueva sesión
```

---

## Estructura del proyecto

```
src/
├── config/         — Pins.h, DeviceConfig.h, FirmwareConfig.h
├── core/           — PresenceService, FeedingSessionService, TimeService
├── camera/         — CameraService (OV2640/OV3660, QVGA JPEG)
├── diagnostics/    — DeviceInfoService
├── dispenser/      — DispenserService, DispenserMotor, RotationSensor, LimitSwitch
├── media/          — MediaUploadService (HTTP POST JPEG al backend)
├── mqtt/           — MqttService, MqttTopics
├── provisioning/   — ProvisioningService
├── rfid/           — RfidReader (ISO 11784/85 FDX-B, 134.2 kHz)
├── scale/          — ScaleSensor (HX711)
├── sensors/        — DistanceSensor (GP2Y0A21YK0F), EnvironmentSensor (DHT22)
├── storage/        — ConfigStorage (NVS flash)
├── telemetry/      — TelemetryBuilder
└── wifi/           — WifiService
```
