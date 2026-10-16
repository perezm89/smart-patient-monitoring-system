const express = require('express');
const { authenticateToken } = require('../middleware/authMiddleware');
const { getMyVitals } = require('../controllers/patientsController');

const router = express.Router();

router.get('/me/vitals', authenticateToken, getMyVitals);

module.exports = router;