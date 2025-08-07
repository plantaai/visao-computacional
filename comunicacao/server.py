from fastapi import FastAPI, File, UploadFile, HTTPException
from fastapi.responses import JSONResponse
from datetime import datetime
import tensorflow as tf
import numpy as np
import uvicorn
import cv2
import os

IMAGE_SIZE = 256
CLASS_NAMES = ['doente', 'saudavel']
MODEL_PATH = "model/"
STORAGE_DIR = "stored_images"
MAX_IMAGES = 30

app = FastAPI(title="API de Classificação de Saúde de Alface")
model = None

os.makedirs(STORAGE_DIR, exist_ok=True)

def carregar_modelo(caminho: str):
    if not os.path.exists(caminho):
        raise FileNotFoundError(f"O diretório do modelo não foi encontrado: {caminho}")
    return tf.keras.models.load_model(caminho)


def carregar_imagem_bytes(image_bytes: bytes):
    nparr = np.frombuffer(image_bytes, np.uint8)
    img = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
    if img is None:
        raise ValueError("Não foi possível decodificar a imagem")
    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    return img


def predict_imagem_sistema_de_arquivo(model, image):
    imageArray = tf.keras.preprocessing.image.img_to_array(image)
    imageArray = tf.expand_dims(imageArray, 0)
    predictions = model.predict(imageArray)[0, 0]
    predicted_class = CLASS_NAMES[1] if predictions >= 0.5 else CLASS_NAMES[0]
    confidence = round(100 * (predictions if predictions >= 0.5 else 1 - predictions), 2)
    return predicted_class, confidence


def save_image_with_metadata(image, predicted_class: str, confidence: float):
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    conf_str = f"{confidence:.2f}".replace('.', '_')
    filename = f"{predicted_class}-{conf_str}-{timestamp}.jpg"
    filepath = os.path.join(STORAGE_DIR, filename)
    img_bgr = cv2.cvtColor(image, cv2.COLOR_RGB2BGR)
    cv2.imwrite(filepath, img_bgr)
    _limit_images(STORAGE_DIR, MAX_IMAGES)


def _limit_images(directory: str, limit: int):
    files = [os.path.join(directory, f) for f in os.listdir(directory) if os.path.isfile(os.path.join(directory, f))]
    if len(files) <= limit:
        return
    files.sort(key=lambda x: os.path.getctime(x))
    to_delete = files[:-limit]
    for f in to_delete:
        try:
            os.remove(f)
        except OSError:
            pass

@app.on_event("startup")
async def startup_event():
    global model
    try:
        model = carregar_modelo(MODEL_PATH)
        print(f"Modelo carregado de: {MODEL_PATH}")
    except Exception as e:
        print(f"Erro ao carregar modelo: {e}")
        raise e

@app.post("/predict")
async def predict_health(file: UploadFile = File(...)):
    if not file.filename.lower().endswith(('.jpg', '.jpeg', '.png')):
        raise HTTPException(status_code=400, detail="Formato de arquivo inválido. Use JPG ou PNG.")
    content = await file.read()
    try:
        image = carregar_imagem_bytes(content)
    except Exception as e:
        raise HTTPException(status_code=400, detail=f"Erro ao processar imagem: {e}")
    try:
        predicted_class, confidence = predict_imagem_sistema_de_arquivo(model, image)
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Erro na predição: {e}")

    try:
        save_image_with_metadata(image, predicted_class, confidence)
    except Exception:
        pass

    return JSONResponse(content={"predicted_class": predicted_class, "confidence": confidence})

if __name__ == "__main__":
    uvicorn.run("server:app", host="0.0.0.0", port=8000, reload=True)
