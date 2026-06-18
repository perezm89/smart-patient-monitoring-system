Smart Patient Monitoring Backend

Endpoint: POST /api/vitals

Purpose:
Receive patient sensor readings from ESP32 device.

Input:
- deviceId
- heartRate
- spo2
- temperature
- fallDetected
- batteryLevel

Process:
1. Receive JSON payload
2. Validate required fields
3. Store reading in MongoDB
4. Check thresholds
5. Generate alert if needed
6. Return success response

Output:
200 OK