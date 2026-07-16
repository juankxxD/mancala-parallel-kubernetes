# Despliegue Kubernetes local (kind / minikube / k3d)

Manifiestos YAML para levantar los 3 componentes en un clúster local.

## Cumplimiento de la rúbrica

- `backend` tiene **3 réplicas** (mínimo exigido).
- Cada contenedor declara `requests` y `limits` de CPU y memoria.
- `ConfigMap` para variables del motor (`OMP_NUM_THREADS`, profundidad por defecto, orígenes CORS).
- Probes `liveness` y `readiness` apuntan a `/healthz` y `/readyz` respectivamente.
- `Service` ClusterIP interno (`motor-svc`) y NodePort para exposición (`backend-svc` en 30080, `frontend-svc` en 30088).

## Reproducir desde cero (kind)

```powershell
# 1. Crear clúster local
kind create cluster --name mancala

# 2. Construir imágenes localmente
docker build -t mancala-motor:dev    ../../motor
docker build -t mancala-backend:dev  ../../backend
docker build -t mancala-frontend:dev ../../frontend

# 3. Cargar imágenes en el clúster kind
kind load docker-image mancala-motor:dev mancala-backend:dev mancala-frontend:dev --name mancala

# 4. Aplicar manifiestos (orden importa por la dependencia del namespace)
kubectl apply -f .

# 5. Verificar
kubectl get pods,svc -n mancala
kubectl logs -n mancala deploy/backend

# 6. Acceder
#   Frontend: http://localhost:30088
#   Backend:  http://localhost:30080
```

## Reproducir con minikube

```powershell
minikube start
minikube image build -t mancala-motor:dev    ../../motor
minikube image build -t mancala-backend:dev  ../../backend
minikube image build -t mancala-frontend:dev ../../frontend
kubectl apply -f .
minikube service frontend-svc -n mancala
```

## Limpieza

```powershell
kubectl delete -f .
# o
kind delete cluster --name mancala
```
