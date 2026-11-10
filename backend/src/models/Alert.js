const mongoose = require('mongoose');

const alertSchema = new mongoose.Schema(
  {
    patientId: {
      type: mongoose.Schema.Types.ObjectId,
      ref: 'User',
      required: true
    },

    deviceId: {
      type: String,
      required: true,
      trim: true
    },

    vitalId: {
      type: mongoose.Schema.Types.ObjectId,
      ref: 'Vital',
      required: true,
      unique: true
    },

    type: {
      type: String,
      enum: ['fall'],
      required: true
    },

    status: {
      type: String,
      enum: ['active', 'acknowledged'],
      default: 'active'
    },

    timestamp: {
      type: Date,
      required: true
    }
  },
  {
    timestamps: true
  }
);

module.exports = mongoose.model('Alert', alertSchema);