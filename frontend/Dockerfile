FROM nginx:alpine

# Reemplazar la configuración por defecto.
RUN rm /etc/nginx/conf.d/default.conf
COPY nginx.conf /etc/nginx/conf.d/default.conf

# Estáticos.
COPY public/ /usr/share/nginx/html/

EXPOSE 80
