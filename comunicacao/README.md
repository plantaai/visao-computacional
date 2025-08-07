# API de Classificação de Saúde de Alface

Este projeto fornece uma API RESTful para classificar imagens de alface em saudáveis ou não saudáveis usando um modelo TensorFlow/Keras.

## Pré-requisitos

-   Python 3.9 instalado (verifique com `python --version` ou `py -3.9 --version`)
-   `requirements.txt` contendo todas as dependências do projeto

## Como usar

1. **Criar ambiente virtual**

    ```bash
    py -3.9 -m venv venv
    ```

2. **Ativar o ambiente**

    ```bash
    venv\Scripts\activate
    ```

3. **Instalar dependências**

    ```bash
    pip install -r requirements.txt
    ```

4. **Iniciar o servidor**

    ```bash
    uvicorn server:app --host 0.0.0.0 --port 8000 --reload
    ```

5. **Testar predições**

    - Imagem de alface doente:

        ```bash
        curl -X POST "http://localhost:8000/predict" -F "file=@entrada/doente1.jpeg"
        ```

    - Imagem de alface saudável:

        ```bash
        curl -X POST "http://localhost:8000/predict" -F "file=@entrada/saudavel1.jpg"
        ```

6. **Desativar o ambiente**

    ```bash
    deactivate
    ```

## Estrutura de pastas

```
├── model/   # Modelo treinado
├── entrada/                                 # Exemplo de imagens de teste
├── stored_images/                            # Imagens armazenadas com metadata
├── server.py                                 # Código da API FastAPI
├── requirements.txt                          # Dependências do projeto
└── README.md                                 # Este arquivo
```
