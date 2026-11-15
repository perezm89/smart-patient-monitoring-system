
require('dotenv').config();

const mongoose = require('mongoose');
const bcrypt = require('bcryptjs');
const crypto = require('crypto');

const Device = require('../src/models/Device');

const DEVICE_ID = 'SIM-ESP32-001';

async function provisionSimulatedDevice() {
  try {
    if (!process.env.MONGODB_URI) {
      throw new Error('MONGODB_URI is not configured');
    }

    await mongoose.connect(process.env.MONGODB_URI);

    const existingDevice = await Device.findOne({
      deviceId: DEVICE_ID
    });

    if (existingDevice) {
      console.log(`Device ${DEVICE_ID} already exists.`);
      console.log('No changes were made.');
      return;
    }

    // Generate a cryptographically secure API key.
    const apiKey = crypto.randomBytes(32).toString('hex');

    // Store only the hash in MongoDB.
    const apiKeyHash = await bcrypt.hash(apiKey, 10);

    const device = await Device.create({
      deviceId: DEVICE_ID,
      deviceType: 'simulated',
      apiKeyHash,
      assignedPatient: null,
      active: true
    });

    console.log('\nSimulated device provisioned successfully!');
    console.log(`Device ID: ${device.deviceId}`);
    console.log(`Device Type: ${device.deviceType}`);
    console.log(`API Key: ${apiKey}`);
    console.log('\nIMPORTANT: Save this API key securely.');
    console.log('It will not be displayed again.');

  } catch (error) {
    console.error('Provisioning failed:', error.message);
    process.exitCode = 1;
  } finally {
    await mongoose.disconnect();
  }
}

provisionSimulatedDevice();
