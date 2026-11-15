
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

    // Unique identifier for independently submitted emergency events.
    // Optional to preserve compatibility with existing vitals-generated alerts.
    eventId: {
      type: String,
      trim: true,
      default: undefined
    },

    // Optional so SOS events can exist without vital-sign measurements.
    vitalId: {
      type: mongoose.Schema.Types.ObjectId,
      ref: 'Vital',
      required: false
    },

    type: {
      type: String,
      enum: ['fall', 'sos'],
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

// Prevent duplicate alerts associated with the same vital record.
// Independent emergency events without a vitalId are excluded.
alertSchema.index(
  { vitalId: 1, type: 1 },
  {
    name: 'vitalId_1_type_1_partial',
    unique: true,
    partialFilterExpression: {
      vitalId: { $type: 'objectId' }
    }
  }
);

// Prevent duplicate emergency events during device retries.
// Existing alerts without an eventId are excluded.
alertSchema.index(
  { eventId: 1 },
  {
    unique: true,
    partialFilterExpression: {
      eventId: { $type: 'string' }
    }
  }
);

module.exports = mongoose.model('Alert', alertSchema);
