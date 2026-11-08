const express = require('express');

const {
  getAssignedPatients,
  getPatientVitals
} = require('../controllers/caregiverController');

const { authenticateToken } = require('../middleware/authMiddleware');

const router = express.Router();

router.get('/me/patients', authenticateToken, getAssignedPatients);

router.get(
  '/me/patients/:patientId/vitals',
  authenticateToken,
  getPatientVitals
);

module.exports = router;