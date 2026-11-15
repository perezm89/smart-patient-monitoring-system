
import { useState } from 'react';
import { Link, useNavigate } from 'react-router-dom';
import api from '../services/api';

function Signup() {
  const navigate = useNavigate();

  // Form input values
  const [formData, setFormData] = useState({
    firstName: '',
    lastName: '',
    email: '',
    password: '',
    role: 'patient',
    phoneNumber: '',
    smsConsent: false
  });

  // Submission state
  const [error, setError] = useState('');
  const [loading, setLoading] = useState(false);

  // Update the corresponding field whenever the user types.
  const handleChange = (event) => {
    const { name, value, type, checked } = event.target;

    setFormData((previous) => ({
      ...previous,
      [name]: type === 'checkbox' ? checked : value
    }));
  };

  // Submit registration information to Express.
  const handleSignup = async (event) => {
    event.preventDefault();

    setError('');
    setLoading(true);

    try {
      const registrationData = {
        ...formData,
        phoneNumber:
          formData.role === 'caregiver'
            ? formData.phoneNumber.trim()
            : '',
        smsConsent:
          formData.role === 'caregiver' &&
          formData.smsConsent &&
          Boolean(formData.phoneNumber.trim())
      };

      await api.post('/api/v1/auth/register', registrationData);

      // Redirect to Login and indicate that registration succeeded.
      navigate('/login', {
        replace: true,
        state: {
          registrationSuccess: true
        }
      });
    } catch (err) {
      setError(
        err.response?.data?.message ||
        'Unable to create account'
      );
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="login-container">
      <form className="login-form" onSubmit={handleSignup}>
        <h1>SPMS</h1>
        <p>Create your account</p>

        <label htmlFor="firstName">First Name</label>
        <input
          id="firstName"
          name="firstName"
          value={formData.firstName}
          onChange={handleChange}
          required
        />

        <label htmlFor="lastName">Last Name</label>
        <input
          id="lastName"
          name="lastName"
          value={formData.lastName}
          onChange={handleChange}
          required
        />

        <label htmlFor="signupEmail">Email</label>
        <input
          id="signupEmail"
          name="email"
          type="email"
          value={formData.email}
          onChange={handleChange}
          required
        />

        <label htmlFor="signupPassword">Password</label>
        <input
          id="signupPassword"
          name="password"
          type="password"
          value={formData.password}
          onChange={handleChange}
          minLength={8}
          required
        />

        <label htmlFor="role">Account Type</label>
        <select
          id="role"
          name="role"
          value={formData.role}
          onChange={handleChange}
        >
          <option value="patient">Patient</option>
          <option value="caregiver">Caregiver</option>
        </select>

        {formData.role === 'caregiver' && (
          <>
            <label htmlFor="phoneNumber">
              Mobile Phone Number (Optional)
            </label>
            <input
              id="phoneNumber"
              name="phoneNumber"
              type="tel"
              placeholder="+14075551234"
              value={formData.phoneNumber}
              onChange={handleChange}
            />

            <div className="sms-consent">
              <label htmlFor="smsConsent">
                <input
                  id="smsConsent"
                  name="smsConsent"
                  type="checkbox"
                  checked={formData.smsConsent}
                  onChange={handleChange}
                  disabled={!formData.phoneNumber.trim()}
                />
                {' '}
                I agree to receive automated emergency SMS
                notifications from Smart Patient Monitoring
                System (SPMS) at the mobile number provided.
                Notifications may include fall detection and
                SOS alerts. Message frequency varies.
                Message and data rates may apply.
                Reply STOP to opt out or HELP for assistance.
                Consent is optional and is not required
                to create an account.
              </label>
              <p>
                Please review our{' '}
                <a href="/privacy" target="_blank" rel="noopener noreferrer">
                  Privacy Policy
                </a>
                {' '}and{' '}
                <a href="/terms" target="_blank" rel="noopener noreferrer">
                  Terms &amp; Conditions
                </a>
                .
              </p>
            </div>
          </>
        )}

        {error && (
          <p className="login-error" role="alert">
            {error}
          </p>
        )}

        <button type="submit" disabled={loading}>
          {loading ? 'Creating Account...' : 'Sign Up'}
        </button>

        <p>
          Already have an account? <Link to="/login">Log In</Link>
        </p>
      </form>
    </div>
  );
}

export default Signup;
