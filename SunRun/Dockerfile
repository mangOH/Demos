FROM python:3-alpine


COPY requirements.txt /build/
RUN pip install --trusted-host pypi.python.org -r /build/requirements.txt

WORKDIR /app
COPY app.py /app/
EXPOSE 8050

# For testing
#CMD ["python", "app.py"]

# Use gunicorn for deployment
CMD gunicorn -w 2 -b 0.0.0.0:8050 -t 100000 --max-requests 20 --reload app:server