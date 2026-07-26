const mongoose = require('mongoose');

const vitalSchema = new mongoose.Schema(
  {
    deviceId: {
      type: String,
      required: true,
      trim: true
    },

    patientId: {
      type: String,
      required: true,
      trim: true
    },

    timestamp: {
      type: Date,
      required: true
    },

    heartRateBpm: {
      type: Number,
      required: true,
      min: 0
    },

    spo2Percent: {
      type: Number,
      required: true,
      min: 0,
      max: 100
    },

    skinTemperatureC: {
      type: Number,
      required: true
    }
  },
  {
    timestamps: true
  }
);

module.exports = mongoose.model('Vital', vitalSchema);