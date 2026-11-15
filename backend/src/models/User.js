const mongoose = require('mongoose');

const userSchema = new mongoose.Schema(
  {
    firstName: {
      type: String,
      required: true,
      trim: true
    },

    lastName: {
      type: String,
      required: true,
      trim: true
    },

    email: {
      type: String,
      required: true,
      unique: true,
      lowercase: true,
      trim: true
    },

    password: {
      type: String,
      required: true
    },

    role: {
      type: String,
      enum: ['patient', 'caregiver'],
      required: true
    },

    phoneNumber: {
      type: String,
      trim: true,
      default: null
    },

    smsConsent: {
      type: Boolean,
      default: false
    },

    smsConsentAt: {
      type: Date,
      default: null
    },

    assignedPatients: [
      {
        type: mongoose.Schema.Types.ObjectId,
        ref: 'User'
      }
    ]
  },
  {
    timestamps: true
  }
);

module.exports = mongoose.model('User', userSchema);