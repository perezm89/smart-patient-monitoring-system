const Vital = require('../models/Vital');

const getMyVitals = async (req, res) => {
    try {
        if (req.user.role !== 'patient') {
            return res.status(403).json({
                success: false,
                message: 'Access denied'
            });
        }

        const vitals = await Vital.find({
            patientId: req.user.userId
        }).sort({ timestamp: -1 });

        return res.status(200).json({
            success: true,
            data: vitals
        });

    } catch (error) {
        console.error('Failed to retrieve vitals:', error.message);

        return res.status(500).json({
            success: false,
            message: 'Internal server error'
        });
    }
};

module.exports = { getMyVitals };