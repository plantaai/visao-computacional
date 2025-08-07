# visao-computacional
Projeto Visão Computacional com ESP32-S3-CAM

- Hardware: ESP32-S3-CAM (5 MP) alimentado por powerbank, botão de disparo, tela OLED e suporte com tripé para posição fixa.
- Comunicação: envio HTTP POST ao servidor local (notebook), que armazena a foto, roda a predição e retorna rótulo + confiança (%) ao ESP32.
- IA: CNN com camadas de convolução, pooling e duas densas; treino em dados rotulados (treino/validação/teste); na inferência, redimensiona para 256×256 e normaliza antes da classificação probabilística.
