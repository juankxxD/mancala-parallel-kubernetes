# Despliegue Kubernetes en la nube

Manifiestos YAML versionados para EKS / AKS / GKE.

## Antes de aplicar

1. **Elegir proveedor** y completar las anotaciones del Ingress en [`50-ingress.yaml`](50-ingress.yaml).
2. **Reemplazar las imágenes** en `20-motor.yaml`, `30-backend.yaml`, `40-frontend.yaml`:
   - sustituir `ghcr.io/<organizacion>/...` por el registro real
   - usar **tags inmutables** (e.g. `v0.2.0`, NO `latest`)
3. **Actualizar `ALLOWED_ORIGINS`** en `10-configmap.yaml` con el dominio real del frontend.
4. **Crear el secreto TLS** o configurar cert-manager para `mancala-tls` (referenciado en el Ingress).

## Aplicar

```bash
kubectl apply -f .
kubectl get pods,svc,deploy,ingress -n mancala
```

## Cumplimiento de la rúbrica

- Toda la configuración del clúster y la aplicación está **versionada en YAML**.
- Balanceo lo maneja el `Service` de Kubernetes (`backend-svc` con `Ingress`).
- **3 réplicas** del backend (configurable por `HorizontalPodAutoscaler`).
- Imágenes en un **registro accesible** con tags **inmutables**.
- `requests` y `limits` declarados en cada contenedor.
- CORS gestionado por el backend con orígenes explícitos.

## Evidencia esperada (capturas en docs/05-despliegue-nube.md)

```bash
kubectl get pods,svc,deploy,ingress -n mancala
kubectl describe deployment backend -n mancala
kubectl top pods -n mancala
```
