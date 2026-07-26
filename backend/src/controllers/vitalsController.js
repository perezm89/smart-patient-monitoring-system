const Vital = require('../models/Vital');

const createVital = async (req, res) => {
    try {
        const vital = new Vital(req.body);

        const savedVital = await vital.save();

        res.status(201).json({
            success: true,
            message: 'Vitals recorded successfully',
            data: savedVital
        });

    } catch (error) {

        res.status(500).json({
            success: false,
            message: error.message
        });

    }
};

module.exports = {
    createVital
};