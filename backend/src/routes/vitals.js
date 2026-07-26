const express = require('express');
const { createVital } = require('../controllers/vitalsController');

const router = express.Router();

router.post('/', createVital);

module.exports = router;