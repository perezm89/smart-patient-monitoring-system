const Vital = require('../models/Vital');

const createVital = async (req, res) => {
    try {
        const {
            timestamp,
            heartRateBpm,
            spo2Percent,
            skinTemperatureC
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
            skinTemperatureC
        });

        const savedVital = await vital.save();

        return res.status(201).json({
            success: true,
            message: 'Vitals recorded successfully',
            data: savedVital
        });

    } catch (error) {
        console.error('Failed to record vitals:', error.message);

        return res.status(500).json({
            success: false,
            message: 'Internal server error'
        });
    }
};

module.exports = {
    createVital
};