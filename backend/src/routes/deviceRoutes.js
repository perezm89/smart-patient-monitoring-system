
const express = require('express');

const {
  registerDevice,
  assignDeviceToPatient,
  getDevices
} = require('../controllers/deviceController');

const { authenticateToken } = require('../middleware/authMiddleware');

const router = express.Router();

// Existing device management endpoints
router.post('/register', registerDevice);
router.post('/assign', assignDeviceToPatient);

// Authenticated device listing
router.get('/', authenticateToken, getDevices);

module.exports = router;
