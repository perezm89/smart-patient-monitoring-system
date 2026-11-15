
const express = require('express');

const {
  registerDevice,
  assignDeviceToPatient,
  getDevices,
  claimDevice
} = require('../controllers/deviceController');

const { authenticateToken } = require('../middleware/authMiddleware');

const router = express.Router();

// Existing device management endpoints
// Device registration disabled until protected provisioning is implemented.
// router.post('/register', registerDevice);
// Legacy assignment endpoint disabled.
// Patients must use the authenticated /claim endpoint.
// router.post('/assign', assignDeviceToPatient);
router.post('/claim', authenticateToken, claimDevice);

// Authenticated device listing
router.get('/', authenticateToken, getDevices);

module.exports = router;
