FROM python:3.11-slim

# Set working directory
WORKDIR /app

# Copy requirements first to leverage Docker cache
COPY Web_page/requirements.txt .

# Install dependencies
RUN pip install --no-cache-dir -r requirements.txt

# Copy the rest of the application
COPY Web_page/ .

# Set environment variables
ENV FLASK_APP=app.py
ENV FLASK_ENV=production

# Expose the port that the app runs on
EXPOSE 8080

# Set a secure secret key for session management
ENV SECRET_KEY="your-secure-secret-key-here"

# Run the application with production settings
CMD ["python", "-m", "flask", "run", "--host=0.0.0.0", "--port=8080"]
