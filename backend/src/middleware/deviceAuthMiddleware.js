const Device = require('../models/Device');
const bcrypt = require('bcryptjs');

const authenticateDevice = async (req, res, next) => {
    try {

        const { deviceId } = req.body;
        const apiKey = req.headers['x-api-key'];

        if (!deviceId || !apiKey) {
            return res.status(401).json({
                success: false,
                message: 'Device credentials required'
            });
        }

        const device = await Device.findOne({ deviceId });

        if (!device) {
            return res.status(401).json({
                success: false,
                message: 'Invalid device credentials'
            });
        }

        const isValidApiKey = await bcrypt.compare(
            apiKey,
            device.apiKeyHash
        );

        if (!isValidApiKey) {
            return res.status(401).json({
                success: false,
                message: 'Invalid device credentials'
            });
        }

        if (!device.active) {
            return res.status(403).json({
                success: false,
                message: 'Device is inactive'
            });
        }

        req.device = device;

        return next();

    } catch (error) {
        console.error('Device authentication failed:', error.message);

        return res.status(500).json({
            success: false,
            message: 'Internal server error'
        });
    }
};

module.exports = {
    authenticateDevice
};