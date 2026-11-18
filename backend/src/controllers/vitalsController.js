
const Vital = require('../models/Vital');
const Alert = require('../models/Alert');
const { notifyCaregivers } = require('../services/caregiverSmsService');
const { sendWhatsApp } = require('../services/whatsappService');

const createVital = async (req, res) => {
    try {
        const {
            timestamp,
            heartRateBpm,
            spo2Percent,
            skinTemperatureC,
            fallDetected,
            sosTriggered
        } = req.body;

        const device = req.device;

        if (!device.assignedPatient) {
            return res.status(400).json({
                success: false,
                message: 'Device is not assigned to a patient'
            });
        }

        const vital = new Vital({
            deviceId: device.deviceId,
            patientId: device.assignedPatient,
            timestamp,
            heartRateBpm,
            spo2Percent,
            skinTemperatureC,
            fallDetected,
            sosTriggered
        });

        const savedVital = await vital.save();

        // Handle fall detection
        if (savedVital.fallDetected === true) {
            const alert = new Alert({
                patientId: savedVital.patientId,
                deviceId: savedVital.deviceId,
                vitalId: savedVital._id,
                type: 'fall',
                timestamp: savedVital.timestamp
            });

            await alert.save();

            console.log(
                `Fall alert created for patient ${savedVital.patientId}`
            );

            // Notify assigned caregivers who opted in to SMS.
            if (process.env.SMS_ENABLED === 'true') {
                void notifyCaregivers(
                    savedVital.patientId,
                    'fall'
                );
            }

            // Send WhatsApp fall alert to the configured Sandbox recipient.
            if (process.env.WHATSAPP_ENABLED === 'true') {
                void sendWhatsApp(
                    `SPMS FALL ALERT: A fall was detected for patient ${savedVital.patientId}. Please check on the patient immediately.`
                );
            }
        }

        // Handle emergency SOS
        if (savedVital.sosTriggered === true) {
            const alert = new Alert({
                patientId: savedVital.patientId,
                deviceId: savedVital.deviceId,
                vitalId: savedVital._id,
                type: 'sos',
                timestamp: savedVital.timestamp
            });

            await alert.save();

            console.log(
                `SOS alert created for patient ${savedVital.patientId}`
            );

            // Notify assigned caregivers who opted in to SMS.
            if (process.env.SMS_ENABLED === 'true') {
                void notifyCaregivers(
                    savedVital.patientId,
                    'sos'
                );
            }

            // Send WhatsApp SOS alert to the configured Sandbox recipient.
            if (process.env.WHATSAPP_ENABLED === 'true') {
                void sendWhatsApp(
                    `SPMS SOS ALERT: Patient ${savedVital.patientId} triggered an emergency SOS. Please check on the patient immediately.`
                );
            }
        }

        return res.status(201).json({
            success: true,
            message: 'Vitals recorded successfully',
            data: savedVital
        });

    } catch (error) {
        console.error(
            'Failed to record vitals:',
            error.message
        );

        return res.status(500).json({
            success: false,
            message: 'Internal server error'
        });
    }
};

module.exports = {
    createVital
};
