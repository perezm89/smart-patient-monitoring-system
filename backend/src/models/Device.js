const mongoose = require('mongoose');

const deviceSchema = new mongoose.Schema(
  {
    deviceId: {
      type: String,
      required: true,
      unique: true,
      trim: true
    },

    assignedPatient: {
      type: mongoose.Schema.Types.ObjectId,
      ref: 'User',
      default: null
    },

    apiKeyHash: {
      type: String,
      required: true
    },

    active: {
      type: Boolean,
      default: true
    }
  },
  {
    timestamps: true
  }
);

module.exports = mongoose.model('Device', deviceSchema);