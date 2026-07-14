# Smart Patient Monitoring System
# REST API Specification (Version 1)

---

# Overview

This document defines the initial REST API endpoints used by the Smart Patient Monitoring System.

The backend provides secure communication between:

- ESP32-C6 wearable device
- MongoDB database
- Patient Dashboard
- Caregiver Dashboard

All protected routes require JSON Web Token (JWT) authentication unless otherwise noted.

Unless specified otherwise, all request and response bodies use JSON.

---

# Authentication

## Register User

**POST** `/api/v1/auth/register`

**Description:**
Creates a new patient or caregiver account.

**Authentication:**
Not Required

---

## Login

**POST** `/api/v1/auth/login`

**Description:**
Authenticates a user and returns a JSON Web Token (JWT).

**Authentication:**
Not Required

---

# Device API

## Upload Vitals

**POST** `/api/v1/vitals`

**Description:**
Receives routine vital-sign readings from the ESP32-C6 wearable device.

**Authentication:**
Device Authentication (Future)

### Request Header

```http
Content-Type: application/json
```

### Request Body

See:

`vitals-payload-v1.json`

### Successful Response

**HTTP Status:** `201 Created`

```json
{
  "success": true,
  "message": "Vital-sign reading stored successfully"
}
```

### Initial Development Note

During initial firmware and backend integration, device authentication is **not** required.

Future versions of the backend will authenticate registered devices before accepting uploaded data.

---

## Upload Device Event

**POST** `/api/v1/events`

**Description:**
Receives emergency events such as:

- Fall Detection
- SOS Button
- Low Battery
- Connection Loss

**Authentication:**
Device Authentication (Future)

---

## Upload Device Status

**POST** `/api/v1/device-status`

**Description:**
Receives battery level, connection status, firmware version, and charging state.

**Authentication:**
Device Authentication (Future)

---

## Bulk Synchronization

**POST** `/api/v1/vitals/bulk`

**Description:**
Uploads readings stored locally while network connectivity was unavailable.

**Authentication:**
Device Authentication (Future)

---

# Patient Dashboard

## Retrieve Own Vitals

**GET** `/api/v1/patients/me/vitals`

**Description:**
Returns current and historical vital-sign readings for the authenticated patient.

**Authentication:**
Patient JWT Required

---

## Retrieve Own Alerts

**GET** `/api/v1/patients/me/alerts`

**Description:**
Returns alerts generated for the authenticated patient.

**Authentication:**
Patient JWT Required

---

## Retrieve Own Device Status

**GET** `/api/v1/patients/me/device-status`

**Description:**
Returns battery level, connection status, firmware version, and charging state.

**Authentication:**
Patient JWT Required

---

# Caregiver Dashboard

## Retrieve Assigned Patients

**GET** `/api/v1/caregivers/me/patients`

**Description:**
Returns patients currently assigned to the authenticated caregiver.

**Authentication:**
Caregiver JWT Required

---

## Retrieve Assigned Patient Vitals

**GET** `/api/v1/caregivers/me/patients/:patientId/vitals`

**Description:**
Returns current and historical vital-sign readings for an assigned patient.

**Authentication:**
Caregiver JWT Required

---

## Retrieve Assigned Patient Alerts

**GET** `/api/v1/caregivers/me/patients/:patientId/alerts`

**Description:**
Returns alert history for an assigned patient.

**Authentication:**
Caregiver JWT Required

---

## Retrieve Assigned Device Status

**GET** `/api/v1/caregivers/me/patients/:patientId/device-status`

**Description:**
Returns the current wearable-device status for an assigned patient.

**Authentication:**
Caregiver JWT Required

---

# System

## Health Check

**GET** `/api/v1/health`

**Description:**
Verifies that the backend service is operational.

**Authentication:**
Not Required

**Successful Response**

**HTTP Status:** `200 OK`

```json
{
  "status": "OK",
  "service": "Smart Patient Monitoring API"
}
```

---

# General API Conventions

## Content Type

All requests that include a body must send:

```http
Content-Type: application/json
```

## Timestamp Format

All timestamps shall use UTC in ISO 8601 format.

Example:

```text
2026-07-12T22:00:00Z
```

## JSON Naming Convention

JSON property names use **camelCase**.

Example:

```json
{
  "heartRateBpm": 72,
  "spo2Percent": 98,
  "skinTemperatureC": 36.8
}
```

Numeric measurements should be transmitted as JSON numbers rather than strings.

---

# Future API Enhancements

Planned features include:

- Email verification
- Password reset
- Refresh JWT tokens
- Device registration
- Firmware update support
- Notification preferences
- Audit logging
- Administrative dashboard