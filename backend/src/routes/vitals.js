const express = require('express');
const { createVital } = require('../controllers/vitalsController');
const { authenticateDevice } = require('../middleware/deviceAuthMiddleware');

const router = express.Router();

router.post('/', authenticateDevice, createVital);

module.exports = router;