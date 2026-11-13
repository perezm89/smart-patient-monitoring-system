
const Device = require('../models/Device');
const User = require('../models/User');
const Vital = require('../models/Vital');
const bcrypt = require('bcryptjs');

const SALT_ROUNDS = 10;

const registerDevice = async (req, res) => {
  try {
    const { deviceId, apiKey } = req.body;

    if (!deviceId || !apiKey) {
      return res.status(400).json({
        message: 'Device ID and API key are required'
      });
    }

    const existingDevice = await Device.findOne({ deviceId });

    if (existingDevice) {
      return res.status(409).json({
        message: 'Device already registered'
      });
    }

    const apiKeyHash = await bcrypt.hash(apiKey, SALT_ROUNDS);

    const device = new Device({
      deviceId,
      apiKeyHash
    });

    await device.save();

    return res.status(201).json({
      message: 'Device registered successfully',
      device: {
        id: device._id,
        deviceId: device.deviceId,
        assignedPatient: device.assignedPatient,
        active: device.active
      }
    });
  } catch (error) {
    console.error('Device registration failed:', error.message);

    return res.status(500).json({
      message: 'Internal server error'
    });
  }
};

const assignDeviceToPatient = async (req, res) => {
  try {
    const { deviceId, patientId } = req.body;

    if (!deviceId || !patientId) {
      return res.status(400).json({
        message: 'Device ID and patient ID are required'
      });
    }

    const device = await Device.findOne({ deviceId });

    if (!device) {
      return res.status(404).json({
        message: 'Device not found'
      });
    }

    const patient = await User.findById(patientId);

    if (!patient) {
      return res.status(404).json({
        message: 'Patient not found'
      });
    }

    if (patient.role !== 'patient') {
      return res.status(400).json({
        message: 'Device can only be assigned to a patient'
      });
    }

    device.assignedPatient = patient._id;

    await device.save();

    return res.status(200).json({
      message: 'Device assigned to patient successfully',
      device: {
        id: device._id,
        deviceId: device.deviceId,
        assignedPatient: device.assignedPatient,
        active: device.active
      }
    });
  } catch (error) {
    console.error('Device assignment failed:', error.message);

    return res.status(500).json({
      message: 'Internal server error'
    });
  }
};

// Retrieve devices belonging to the authenticated patient
// or patients assigned to the authenticated caregiver.
const getDevices = async (req, res) => {
  try {
    const user = await User.findById(req.user.userId);

    if (!user) {
      return res.status(404).json({
        success: false,
        message: 'User not found'
      });
    }

    let patientIds;

    if (user.role === 'patient') {
      patientIds = [user._id];
    } else if (user.role === 'caregiver') {
      const assignedPatients = await User.find({
        _id: { $in: user.assignedPatients || [] },
        role: 'patient'
      }).select('_id');

      patientIds = assignedPatients.map(patient => patient._id);
    } else {
      return res.status(403).json({
        success: false,
        message: 'Access denied'
      });
    }

    const devices = await Device.find({
      assignedPatient: { $in: patientIds }
    })
      .select('deviceId assignedPatient active createdAt')
      .populate('assignedPatient', 'firstName lastName')
      .sort({ createdAt: -1 })
      .lean();

    const deviceData = await Promise.all(
      devices.map(async device => {
        const latestVital = await Vital.findOne({
          deviceId: device.deviceId,
          patientId: device.assignedPatient._id
        })
          .sort({ timestamp: -1 })
          .select('timestamp')
          .lean();

        return {
          id: device._id,
          deviceId: device.deviceId,
          active: device.active,
          assignedPatient: {
            id: device.assignedPatient._id,
            firstName: device.assignedPatient.firstName,
            lastName: device.assignedPatient.lastName
          },
          registeredAt: device.createdAt,
          lastReadingAt: latestVital
            ? latestVital.timestamp
            : null
        };
      })
    );

    return res.status(200).json({
      success: true,
      data: deviceData
    });
  } catch (error) {
    console.error('Failed to retrieve devices:', error.message);

    return res.status(500).json({
      success: false,
      message: 'Internal server error'
    });
  }
};

module.exports = {
  registerDevice,
  assignDeviceToPatient,
  getDevices
};
