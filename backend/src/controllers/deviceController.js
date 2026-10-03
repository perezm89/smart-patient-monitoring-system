const Device = require('../models/Device');
const User = require('../models/User');
const bcrypt = require('bcryptjs');

const SALT_ROUNDS = 10;

const registerDevice = async (req, res) => {
  try {
    const {
      deviceId,
      apiKey
    } = req.body;

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
    const {
      deviceId,
      patientId
    } = req.body;

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


module.exports = {
  registerDevice,
  assignDeviceToPatient
};