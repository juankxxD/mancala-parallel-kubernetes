# frontend/

Contenedor 3 — cliente web servido por `nginx` (HTML/JS o React build
estática).

Archivo en la raíz del directorio:

- `Dockerfile` — recomendable multi-stage (build estático ➜ `nginx`).

El cliente se ejecuta en el navegador del usuario y consume la API
pública del `backend`. Recuerden que el navegador aplica la *Same-Origin
Policy*: el `backend` debe declarar una política de **CORS** con los
orígenes del `frontend` en local y en la nube.
