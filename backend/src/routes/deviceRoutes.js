const express = require('express');

const {
  registerDevice,
  assignDeviceToPatient
} = require('../controllers/deviceController');

const router = express.Router();

router.post('/register', registerDevice);

router.post('/assign', assignDeviceToPatient);

module.exports = router;