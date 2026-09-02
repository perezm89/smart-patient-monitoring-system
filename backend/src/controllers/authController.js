const User = require('../models/User');
const bcrypt = require('bcryptjs');
const jwt = require('jsonwebtoken');

const SALT_ROUNDS = 10;

const registerUser = async (req, res) => {
  try {
    const {
      firstName,
      lastName,
      email,
      password,
      role
    } = req.body;

    if (!firstName || !lastName || !email || !password || !role) {
      return res.status(400).json({
        message: 'All fields are required'
      });
    }

    const existingUser = await User.findOne({ email });

    if (existingUser) {
      return res.status(409).json({
        message: 'Email already registered'
      });
    }

    const hashedPassword = await bcrypt.hash(password, SALT_ROUNDS);

    const user = new User({
      firstName,
      lastName,
      email,
      password: hashedPassword,
      role
    });

    await user.save();

    return res.status(201).json({
      message: 'User registered successfully',
      user: {
        id: user._id,
        firstName: user.firstName,
        lastName: user.lastName,
        email: user.email,
        role: user.role
      }
    });

  } catch (error) {
    console.error('User registration failed:', error.message);

    return res.status(500).json({
      message: 'Internal server error'
    });
  }
};

const loginUser = async (req, res) => {
  try {
    const {
      email,
      password
    } = req.body;
    
    if (!email || !password) {
      return res.status(400).json({
        message: 'Email and password are required'
      }); 
    }

    const user = await User.findOne({ email });

    if (!user) {
      return res.status(401).json({
        message: 'Invalid email or password'
      });
    }

    const passwordMatches = await bcrypt.compare(
      password,
      user.password
    );

    if (!passwordMatches) {
      return res.status(401).json({
        message: 'Invalid email or password'
      });
    }

    const tokenPayload = {
      userId: user._id,
      role: user.role
    };

    const token = jwt.sign(
      tokenPayload,
      process.env.JWT_SECRET,
      { expiresIn: '7d' }
    );

    return res.status(200).json({
      message: 'Login successful',
      token,
      user: {
        id: user._id,
        firstName: user.firstName,
        lastName: user.lastName,
        email: user.email,
        role: user.role
        }
    });

  } catch (error) {
    console.error('User login failed:', error.message);

    return res.status(500).json({
      message: 'Internal server error'
    });
  }
};

module.exports = {
  registerUser,
  loginUser
};