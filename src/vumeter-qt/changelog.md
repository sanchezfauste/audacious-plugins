# Changelog VU Meter Qt

Fecha: 2026-04-24

## Cambios implementados

### 1) Nuevas preferencias de respuesta temporal
Archivo: src/vumeter-qt/vumeter_qt.cc

- Se agregaron dos opciones configurables en preferencias:
  - attack_time (segundos)
  - release_time (segundos)
- Se añadieron sus valores por defecto:
  - attack_time = 0.010
  - release_time = 0.220

Motivo:
- Permitir controlar la velocidad de subida y bajada del medidor para reducir saltos bruscos y acercar el comportamiento a un VU más natural.

### 2) Captura de picos PCM más segura y eficiente
Archivo: src/vumeter-qt/vumeter_qt_widget.cc

- En render_multi_pcm:
  - Se agregó validación temprana cuando channels <= 0.
  - Se reemplazó la reserva dinámica por un buffer local fijo (peaks[max_channels]).
  - Se corrigió el recorrido de muestras usando índice sample * channels + channel.
  - Se acumula en channels_input_peak para procesar luego en el timer.

Motivo:
- Evitar asignaciones/liberaciones por bloque de audio.
- Corregir el acceso a muestras intercaladas por canal.
- Separar adquisición de picos de la lógica visual temporal para una respuesta estable.

### 3) Suavizado de nivel con ataque/liberación
Archivo: src/vumeter-qt/vumeter_qt_widget.cc

- En redraw_timer_expired:
  - Se incorporó dt (tiempo transcurrido real por frame).
  - Se leen attack_time y release_time desde preferencias.
  - Se aplican mínimos de seguridad para evitar divisiones extremas.
  - Se calcula una envolvente por canal con coeficientes exponenciales (attack_alpha/release_alpha).
  - Se convierte la envolvente a dB y se combina con la caída (falloff).
  - Se mantiene la lógica de peak hold por tiempo.

Motivo:
- Eliminar movimiento abrupto cuadro a cuadro.
- Hacer la dinámica del medidor independiente de pequeñas variaciones de frame-time.
- Mantener picos legibles sin perder fluidez visual.

### 4) Inicialización completa del nuevo estado interno
Archivos: src/vumeter-qt/vumeter_qt_widget.h y src/vumeter-qt/vumeter_qt_widget.cc

- Se añadieron nuevos arrays de estado por canal:
  - channels_input_peak
  - channels_envelope
- En reset se inicializan estos estados junto con channels_db_level y channels_peaks.

Motivo:
- Garantizar un estado consistente tras crear el widget o reiniciar reproducción.
- Evitar arrastres de valores anteriores en la etapa de suavizado.

### 5) Caché de capa estática para reducir repintado
Archivos: src/vumeter-qt/vumeter_qt_widget.h y src/vumeter-qt/vumeter_qt_widget.cc

- Se agregó una capa estática cacheada:
  - QPixmap static_layer
  - bool static_layer_dirty
  - método update_static_layer()
- En update_sizes se marca la capa estática como sucia.
- En paintEvent:
  - Se actualiza la capa estática solo cuando es necesario.
  - Se dibuja primero la capa cacheada.
  - Luego se superponen únicamente los elementos dinámicos (barras y picos).

Motivo:
- Evitar redibujar fondo y leyenda en cada frame.
- Reducir trabajo de CPU/GPU por repintado completo.
- Mejorar rendimiento visual en refrescos frecuentes.

### 6) Ajustes de estilo y consistencia con utilidades del proyecto
Archivos: src/vumeter-qt/vumeter_qt_widget.cc

- Uso de utilidades aud::max y aud::abs en rutas tocadas.
- Uso consistente de literales float donde corresponde.
- Conservación del valor por defecto de falloff en 13.3.

Motivo:
- Mantener consistencia con el estilo del proyecto y evitar conversiones implícitas innecesarias.
- Preservar el comportamiento esperado de caída del nivel según la calibración actual.

## Resultado esperado

- Menor costo de render por frame.
- Movimiento del VU más suave y realista.
- Mejor control del comportamiento desde preferencias sin romper compatibilidad de configuración existente.
