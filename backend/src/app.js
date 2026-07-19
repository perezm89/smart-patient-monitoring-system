// Import required packages
const express = require('express');
const cors = require('cors');
const helmet = require('helmet');
const morgan = require('morgan');
const mongoose = require('mongoose');
const vitalsRoutes = require('./routes/vitals');

// Create the Express application instance
const app = express();

/*These middleware functions execute before the request reaches any route.
| They are applied to every incoming HTTP request.
*/

// Allow requests from the frontend or other approved clients
app.use(cors());
// Adds common HTTP security headers
app.use(helmet());
// Log each HTTP request to the terminal for debugging
app.use(morgan('dev'));
// Automatically parse incoming JSON request bodies
app.use(express.json());
app.use('/api/v1/vitals', vitalsRoutes);

/*
  Used to verify that the backend server is running correctly.
| Also reports the current MongoDB connection status.
*/
app.get('/api/v1/health', (req, res) => {

    const databaseConnected = mongoose.connection.readyState === 1;

    res.status(200).json({
        status: 'OK',
        service: 'Smart Patient Monitoring API',
        database: databaseConnected ? 'connected' : 'disconnected',
        timestamp: new Date().toISOString()
    });

});
// Export the Express application for use by server.js
module.exports = app;