
const User = require('../models/User');
const Vital = require('../models/Vital');
const mongoose = require('mongoose');

const getAssignedPatients = async (req, res) => {
  try {
    // Find the authenticated user in MongoDB.
    const caregiver = await User.findById(req.user.userId);

    if (!caregiver) {
      return res.status(404).json({
        success: false,
        message: 'User not found'
      });
    }

    // Only caregivers may access this endpoint.
    if (caregiver.role !== 'caregiver') {
      return res.status(403).json({
        success: false,
        message: 'Access denied'
      });
    }

    // Retrieve only assigned users who are patients.
    const patients = await User.find({
      _id: { $in: caregiver.assignedPatients },
      role: 'patient'
    }).select('_id firstName lastName');

    return res.status(200).json({
      success: true,
      data: patients
    });

  } catch (error) {
    console.error('Failed to retrieve assigned patients:', error.message);

    return res.status(500).json({
      success: false,
      message: 'Internal server error'
    });
  }
};

const getPatientVitals = async (req, res) => {
  try {
    const { patientId } = req.params;

    // Validate the patient ID format.
    if (!mongoose.isObjectIdOrHexString(patientId)) {
      return res.status(400).json({
        success: false,
        message: 'Invalid patient ID'
      });
    }

    // Retrieve the authenticated caregiver.
    const caregiver = await User.findById(req.user.userId);

    if (!caregiver) {
      return res.status(404).json({
        success: false,
        message: 'User not found'
      });
    }

    // Only caregivers may access this endpoint.
    if (caregiver.role !== 'caregiver') {
      return res.status(403).json({
        success: false,
        message: 'Access denied'
      });
    }

    // Verify that this patient is assigned to the caregiver.
    const isAssigned = caregiver.assignedPatients.some(
      id => id.equals(patientId)
    );

    if (!isAssigned) {
      return res.status(403).json({
        success: false,
        message: 'Patient is not assigned to this caregiver'
      });
    }

    // Retrieve the assigned patient's measurements.
    const vitals = await Vital.find({
      patientId: patientId
    })
      .sort({ timestamp: -1 })
      .limit(100);

    return res.status(200).json({
      success: true,
      data: vitals
    });

  } catch (error) {
    console.error('Failed to retrieve patient vitals:', error.message);

    return res.status(500).json({
      success: false,
      message: 'Internal server error'
    });
  }
};

module.exports = {
  getAssignedPatients,
  getPatientVitals
};
