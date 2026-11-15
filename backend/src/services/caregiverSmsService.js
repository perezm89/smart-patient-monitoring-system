
const User = require('../models/User');
const { sendSMS } = require('./smsService');

/**
 * Send an emergency SMS to caregivers assigned to a patient
 * who have explicitly opted in to SMS notifications.
 */
const notifyCaregivers = async (patientId, alertType) => {
  if (process.env.SMS_ENABLED !== 'true') {
    console.log('[SMS] Disabled - caregiver notifications skipped');
    return;
  }

  try {
    const caregivers = await User.find({
      role: 'caregiver',
      assignedPatients: patientId,
      smsConsent: true,
      phoneNumber: { $type: 'string', $ne: '' }
    }).select('phoneNumber');

    if (caregivers.length === 0) {
      console.log('[SMS] No opted-in caregivers found');
      return;
    }

    const message =
      alertType === 'fall'
        ? `SPMS FALL ALERT: A fall was detected for patient ${patientId}. Please check on the patient.`
        : `SPMS SOS ALERT: Patient ${patientId} triggered an emergency SOS. Please check on the patient immediately.`;

    const results = await Promise.allSettled(
      caregivers.map((caregiver) =>
        sendSMS(caregiver.phoneNumber, message)
      )
    );

    const accepted = results.filter(
      (result) =>
        result.status === 'fulfilled' &&
        result.value?.sent === true
    ).length;

    console.log(
      `[SMS] ${accepted}/${caregivers.length} caregiver messages submitted`
    );
  } catch (error) {
    console.error(
      '[SMS] Failed to notify caregivers:',
      error.message
    );
  }
};

module.exports = { notifyCaregivers };
