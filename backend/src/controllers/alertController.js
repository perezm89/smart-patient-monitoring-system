const mongoose = require('mongoose');
const Alert = require('../models/Alert');
const User = require('../models/User');

// Get patient IDs the authenticated user can access.
const getAuthorizedPatientIds = (user) => {
  if (user.role === 'patient') {
    return [user._id];
  }

  if (user.role === 'caregiver') {
    return user.assignedPatients;
  }

  return [];
};

// GET /api/v1/alerts
const getAlerts = async (req, res) => {
  try {
    const user = await User.findById(req.user.userId);

    if (!user) {
      return res.status(404).json({
        success: false,
        message: 'User not found'
      });
    }

    const patientIds = getAuthorizedPatientIds(user);

    const alerts = await Alert.find({
      patientId: { $in: patientIds }
    })
      .sort({ timestamp: -1 })
      .limit(100);

    return res.status(200).json({
      success: true,
      data: alerts
    });

  } catch (error) {
    console.error('Failed to retrieve alerts:', error.message);

    return res.status(500).json({
      success: false,
      message: 'Internal server error'
    });
  }
};

// GET /api/v1/alerts/:id
const getAlertById = async (req, res) => {
  try {
    const { id } = req.params;

    if (!mongoose.isObjectIdOrHexString(id)) {
      return res.status(400).json({
        success: false,
        message: 'Invalid alert ID'
      });
    }

    const user = await User.findById(req.user.userId);

    if (!user) {
      return res.status(404).json({
        success: false,
        message: 'User not found'
      });
    }

    const patientIds = getAuthorizedPatientIds(user);

    const alert = await Alert.findOne({
      _id: id,
      patientId: { $in: patientIds }
    });

    if (!alert) {
      return res.status(404).json({
        success: false,
        message: 'Alert not found'
      });
    }

    return res.status(200).json({
      success: true,
      data: alert
    });

  } catch (error) {
    console.error('Failed to retrieve alert:', error.message);

    return res.status(500).json({
      success: false,
      message: 'Internal server error'
    });
  }
};

// PATCH /api/v1/alerts/:id/acknowledge
const acknowledgeAlert = async (req, res) => {
  try {
    const { id } = req.params;

    if (!mongoose.isObjectIdOrHexString(id)) {
      return res.status(400).json({
        success: false,
        message: 'Invalid alert ID'
      });
    }

    const user = await User.findById(req.user.userId);

    if (!user) {
      return res.status(404).json({
        success: false,
        message: 'User not found'
      });
    }

    const patientIds = getAuthorizedPatientIds(user);

    const alert = await Alert.findOneAndUpdate(
      {
        _id: id,
        patientId: { $in: patientIds }
      },
      {
        $set: { status: 'acknowledged' }
      },
      {
        new: true,
        runValidators: true
      }
    );

    if (!alert) {
      return res.status(404).json({
        success: false,
        message: 'Alert not found'
      });
    }

    return res.status(200).json({
      success: true,
      message: 'Alert acknowledged successfully',
      data: alert
    });

  } catch (error) {
    console.error('Failed to acknowledge alert:', error.message);

    return res.status(500).json({
      success: false,
      message: 'Internal server error'
    });
  }
};

module.exports = {
  getAlerts,
  getAlertById,
  acknowledgeAlert
};