const express = require('express');
const cors = require('cors');
const helmet = require('helmet');
const morgan = require('morgan');

const app = express();

// Middleware
app.use(cors());
app.use(helmet());
app.use(morgan('dev'));
app.use(express.json());

// Test Route
app.get('/', (req, res) => {
    res.json({
        message: 'Smart Patient Monitoring API Running'
    });
});

module.exports = app;