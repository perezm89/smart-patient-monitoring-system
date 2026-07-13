# Smart Patient Monitoring Backend Pseudocode

## 1. System Overview

The backend server will support communication among the following system components:

* ESP32-C6 wearable device
* MongoDB database
* Patient web dashboard
* Caregiver web dashboard
* Twilio SMS notification service

### Selected Device Hardware

* ESP32-C6 microcontroller
* MAX30102 heart-rate and SpO₂ sensor
* BMI270 6-axis IMU for fall detection
* TMP117 contact temperature sensor

### Backend Technologies

* Node.js
* Express.js
* MongoDB
* Mongoose
* JSON Web Tokens
* Password hashing
* Role-based access control
* Twilio SMS notifications

### User Roles

The system will support two primary user roles:

* `patient`
* `caregiver`

Patients may view only their own health, device, and alert information.

Caregivers may view information only for patients who are assigned to their care.

---

# 2. Vitals Upload Endpoint

**Endpoint:** `POST /api/v1/vitals`

**Purpose:**
Receive routine physiological measurements from the ESP32-C6 wearable device.

## Expected Input

* `deviceId`
* `patientId`
* `timestamp`
* `heartRateBpm`
* `spo2Percent`
* `skinTemperatureC`

## Process

1. Receive the JSON payload from the ESP32-C6.
2. Verify that all required fields are present.
3. Validate the data type of each field.
4. Validate each measurement against acceptable technical ranges.
5. Verify that the device is registered.
6. Verify that the device is assigned to the specified patient.
7. Check whether the reading has already been stored.
8. Store the reading in MongoDB using the original UTC timestamp.
9. Evaluate the reading against configured safety thresholds.
10. Determine whether the abnormal condition is sustained or isolated.
11. If an alert condition is met:

    * Create an alert record.
    * Assign an alert type.
    * Assign a severity level.
    * Identify the assigned caregiver.
    * Send an SMS notification when required.
    * Record the notification result.
12. Return a response to the wearable device.

## Successful Response

**HTTP Status:** `201 Created`

```json
{
  "success": true,
  "message": "Vital-sign reading stored successfully"
}
```

## Invalid Payload Response

**HTTP Status:** `400 Bad Request`

```json
{
  "success": false,
  "message": "Invalid or incomplete vital-sign payload"
}
```

## Unknown Device Response

**HTTP Status:** `404 Not Found`

```json
{
  "success": false,
  "message": "Device is not registered"
}
```

---

# 3. Device Event Endpoint

**Endpoint:** `POST /api/v1/events`

**Purpose:**
Receive urgent or event-based information from the wearable device.

## Supported Event Types

* `FALL_DETECTED`
* `SOS_PRESSED`
* `LOW_BATTERY`
* `CONNECTION_LOST`
* `CONNECTION_RESTORED`

## Expected Input

* `deviceId`
* `patientId`
* `timestamp`
* `eventType`
* `severity`
* `message`

## Process

1. Receive the event payload.
2. Verify that all required fields are present.
3. Validate the event type and severity.
4. Verify that the device is registered.
5. Verify that the device is assigned to the specified patient.
6. Store the event in MongoDB.
7. Determine whether the event requires immediate caregiver notification.
8. If notification is required:

   * Retrieve the assigned caregiver.
   * Generate an SMS message.
   * Send the message using Twilio.
   * Record whether delivery succeeded or failed.
9. Return an acknowledgment to the wearable device.

## Successful Response

**HTTP Status:** `201 Created`

```json
{
  "success": true,
  "message": "Device event recorded successfully"
}
```

---

# 4. Device Status Endpoint

**Endpoint:** `POST /api/v1/device-status`

**Purpose:**
Receive operational information about the wearable device.

## Expected Input

* `deviceId`
* `timestamp`
* `batteryPercent`
* `charging`
* `connectionStatus`
* `firmwareVersion`

## Process

1. Receive the device-status payload.
2. Verify that all required fields are present.
3. Validate all field values.
4. Verify that the device is registered.
5. Update the latest device-status record in MongoDB.
6. Update the device's last-seen timestamp.
7. If the battery level is below the configured threshold:

   * Create a low-battery alert if one is not already active.
8. If the connection status has changed:

   * Record the status transition.
9. Return a success response.

## Successful Response

**HTTP Status:** `200 OK`

```json
{
  "success": true,
  "message": "Device status updated successfully"
}
```

---

# 5. Bulk Vitals Synchronization Endpoint

**Endpoint:** `POST /api/v1/vitals/bulk`

**Purpose:**
Receive multiple readings that were stored locally while the wearable device had no network connection.

## Expected Input

* `deviceId`
* `patientId`
* `readings`

Each item in `readings` will contain:

* `timestamp`
* `heartRateBpm`
* `spo2Percent`
* `skinTemperatureC`

## Process

1. Receive the bulk-upload payload.
2. Verify that the device is registered.
3. Verify that the device is assigned to the specified patient.
4. Confirm that `readings` is a non-empty array.
5. Validate every reading.
6. Check for duplicates using:

   * Device ID
   * Patient ID
   * Timestamp
7. Store all valid, non-duplicate readings in MongoDB.
8. Evaluate each stored reading against configured safety thresholds.
9. Create alerts only when the alert logic determines that notification is appropriate.
10. Count:

    * Received readings
    * Saved readings
    * Duplicate readings
    * Rejected readings
11. Return the synchronization result.
12. The firmware may remove synchronized records from local storage only after receiving a successful acknowledgment.

## Successful Response

**HTTP Status:** `201 Created`

```json
{
  "success": true,
  "receivedCount": 20,
  "savedCount": 19,
  "duplicateCount": 1,
  "rejectedCount": 0,
  "message": "Stored readings synchronized successfully"
}
```

---

# 6. User Registration Endpoint

**Endpoint:** `POST /api/v1/auth/register`

**Purpose:**
Create a patient or caregiver account.

## Expected Input

* `name`
* `email`
* `password`
* `role`

## Process

1. Receive registration information.
2. Verify that all required fields are present.
3. Validate the email format.
4. Validate the requested role.
5. Check whether the email address is already registered.
6. Hash the password.
7. Create the user record in MongoDB.
8. Return a success response.

## Successful Response

**HTTP Status:** `201 Created`

```json
{
  "success": true,
  "message": "Account created successfully"
}
```

---

# 7. User Login Endpoint

**Endpoint:** `POST /api/v1/auth/login`

**Purpose:**
Authenticate a patient or caregiver.

## Expected Input

* `email`
* `password`

## Process

1. Receive login credentials.
2. Find the user by email address.
3. Compare the submitted password with the stored password hash.
4. If the credentials are valid:

   * Generate a JWT.
   * Include the user ID and role in the token claims.
   * Return the token and basic account information.
5. If the credentials are invalid:

   * Return an authentication error.

## Successful Response

**HTTP Status:** `200 OK`

```json
{
  "success": true,
  "token": "JWT_TOKEN",
  "user": {
    "id": "USER-001",
    "role": "patient"
  }
}
```

## Invalid Credentials Response

**HTTP Status:** `401 Unauthorized`

```json
{
  "success": false,
  "message": "Invalid email or password"
}
```

---

# 8. Patient Dashboard: Retrieve Own Vitals

**Endpoint:** `GET /api/v1/patients/me/vitals`

**Purpose:**
Allow an authenticated patient to retrieve only their own current and historical vital-sign readings.

## Supported Query Parameters

* `range=1h`
* `range=12h`
* `range=24h`
* `start`
* `end`
* `limit`

## Process

1. Verify the JWT.
2. Confirm that the authenticated user has the `patient` role.
3. Determine the patient ID from the authenticated account.
4. Do not accept an arbitrary patient ID from the request.
5. Validate the requested time range.
6. Query MongoDB for the patient's readings.
7. Sort readings by timestamp.
8. Return the readings to the patient dashboard.

## Successful Response

**HTTP Status:** `200 OK`

```json
{
  "success": true,
  "count": 2,
  "readings": [
    {
      "timestamp": "2026-07-12T22:00:00Z",
      "heartRateBpm": 72,
      "spo2Percent": 98,
      "skinTemperatureC": 36.8
    },
    {
      "timestamp": "2026-07-12T22:00:05Z",
      "heartRateBpm": 74,
      "spo2Percent": 98,
      "skinTemperatureC": 36.8
    }
  ]
}
```

---

# 9. Patient Dashboard: Retrieve Own Alerts

**Endpoint:** `GET /api/v1/patients/me/alerts`

**Purpose:**
Allow an authenticated patient to retrieve only their own alert history.

## Process

1. Verify the JWT.
2. Confirm that the authenticated user has the `patient` role.
3. Determine the patient ID from the authenticated account.
4. Query MongoDB for alerts associated with that patient.
5. Sort alerts from newest to oldest.
6. Return the alert records.

---

# 10. Patient Dashboard: Retrieve Own Device Status

**Endpoint:** `GET /api/v1/patients/me/device-status`

**Purpose:**
Allow an authenticated patient to view the status of their assigned wearable device.

## Process

1. Verify the JWT.
2. Confirm that the authenticated user has the `patient` role.
3. Determine the patient's assigned device.
4. Retrieve the device's:

   * Connection status
   * Battery percentage
   * Charging status
   * Last-seen timestamp
   * Firmware version
5. Return the device-status information.

---

# 11. Caregiver Dashboard: Retrieve Assigned Patients

**Endpoint:** `GET /api/v1/caregivers/me/patients`

**Purpose:**
Allow an authenticated caregiver to retrieve the patients currently assigned to their care.

## Process

1. Verify the JWT.
2. Confirm that the authenticated user has the `caregiver` role.
3. Query the caregiver-patient relationship records.
4. Retrieve only active patient assignments.
5. Return a summary of each assigned patient.

---

# 12. Caregiver Dashboard: Retrieve Assigned Patient Vitals

**Endpoint:** `GET /api/v1/caregivers/me/patients/:patientId/vitals`

**Purpose:**
Allow a caregiver to retrieve current and historical readings for a patient assigned to their care.

## Supported Query Parameters

* `range=1h`
* `range=12h`
* `range=24h`
* `start`
* `end`
* `limit`

## Process

1. Verify the JWT.
2. Confirm that the authenticated user has the `caregiver` role.
3. Verify that the requested patient is assigned to that caregiver.
4. If the patient is not assigned to the caregiver:

   * Return `403 Forbidden`.
5. Validate the requested time range.
6. Query MongoDB for the patient's readings.
7. Sort readings by timestamp.
8. Return the readings to the caregiver dashboard.

## Unauthorized Patient Access Response

**HTTP Status:** `403 Forbidden`

```json
{
  "success": false,
  "message": "You are not authorized to view this patient"
}
```

---

# 13. Caregiver Dashboard: Retrieve Assigned Patient Alerts

**Endpoint:** `GET /api/v1/caregivers/me/patients/:patientId/alerts`

**Purpose:**
Allow a caregiver to retrieve the alert history of an assigned patient.

## Process

1. Verify the JWT.
2. Confirm that the authenticated user has the `caregiver` role.
3. Verify that the requested patient is assigned to the caregiver.
4. Query MongoDB for the patient's alerts.
5. Sort alerts from newest to oldest.
6. Return the alert history.

---

# 14. Caregiver Dashboard: Retrieve Assigned Patient Device Status

**Endpoint:** `GET /api/v1/caregivers/me/patients/:patientId/device-status`

**Purpose:**
Allow a caregiver to view the wearable-device status of an assigned patient.

## Process

1. Verify the JWT.
2. Confirm that the authenticated user has the `caregiver` role.
3. Verify that the requested patient is assigned to the caregiver.
4. Retrieve the patient's assigned device.
5. Return:

   * Connection status
   * Battery percentage
   * Charging status
   * Last-seen timestamp
   * Firmware version

---

# 15. Caregiver-Patient Assignment

The backend will maintain a relationship between caregivers and patients.

## Example Relationship

```text
caregiverId
patientId
status
createdAt
```

## Authorization Process

For caregiver requests involving a specific patient:

1. Authenticate the caregiver.
2. Retrieve the caregiver's ID from the JWT.
3. Search for an active caregiver-patient relationship.
4. Confirm that the requested patient ID matches an active assignment.
5. Allow or reject the request.

A caregiver must not be able to retrieve information for every patient in the database.

---

# 16. Backend Health Check

**Endpoint:** `GET /api/v1/health`

**Purpose:**
Confirm that the backend service is running.

## Process

1. Receive the health-check request.
2. Return the current service status.
3. Return the current UTC timestamp.
4. Optionally return the database connection status.

## Successful Response

**HTTP Status:** `200 OK`

```json
{
  "status": "OK",
  "service": "Smart Patient Monitoring API",
  "database": "connected",
  "timestamp": "2026-07-12T22:00:00Z"
}
```

---

# 17. General Authentication and Authorization

Protected dashboard routes will require a valid JWT.

## Authentication Process

1. Read the authorization header.
2. Extract the JWT.
3. Verify the token signature.
4. Retrieve the user ID and role from the token.
5. Attach the authenticated user information to the request.
6. Continue to the requested route.

## Authorization Process

1. Determine the role required by the route.
2. Compare the authenticated user's role with the required role.
3. Apply patient ownership or caregiver-assignment checks.
4. Return `403 Forbidden` when access is not permitted.

---

# 18. General Error Handling

For every endpoint:

1. Catch unexpected server errors.
2. Record the error through the backend logger.
3. Do not expose:

   * Database credentials
   * JWT secrets
   * Password hashes
   * Internal stack traces
4. Return a consistent JSON error response.

## Server Error Response

**HTTP Status:** `500 Internal Server Error`

```json
{
  "success": false,
  "message": "Internal server error"
}
```

---

# 19. Timestamp Convention

All timestamps transmitted by the ESP32-C6 and stored by the backend will use UTC in ISO 8601 format.

## Example

```text
2026-07-12T22:00:00Z
```

The patient and caregiver dashboards will convert UTC timestamps into the user's local time zone for display.

---

# 20. Initial Implementation Order

The backend will be implemented in the following order:

1. Health-check endpoint
2. MongoDB connection
3. User model
4. Authentication and JWT middleware
5. Patient and caregiver role authorization
6. Device model
7. Caregiver-patient assignment model
8. Vitals model
9. Alert and event models
10. Vitals upload endpoint
11. Device-event endpoint
12. Device-status endpoint
13. Patient dashboard endpoints
14. Caregiver dashboard endpoints
15. SMS notification service
16. Historical data queries
17. Bulk synchronization endpoint
