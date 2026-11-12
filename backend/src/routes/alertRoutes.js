const express = require('express');
const router = express.Router();

const {
    getAlerts,
    getAlertById,
    acknowledgeAlert
} = require('../controllers/alertController');

const {
    authenticateToken
} = require('../middleware/authMiddleware');

// All alert endpoints require JWT authentication.
router.use(authenticateToken);

// Retrieve alerts for authorized patients.
router.get('/', getAlerts);

// Retrieve a specific authorized alert.
router.get('/:id', getAlertById);

// Acknowledge an authorized alert.
router.patch('/:id/acknowledge', acknowledgeAlert);

module.exports = router;