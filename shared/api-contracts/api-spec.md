POST /api/vitals

Description:
Receives sensor readings from ESP32 device.

Response:
{
  "success": true,
  "message": "Vitals recorded successfully"
}

POST /api/alerts/sos

Description:
Receives SOS button activation from wearable device.

Response:
{
  "success": true,
  "message": "SOS alert received"
}