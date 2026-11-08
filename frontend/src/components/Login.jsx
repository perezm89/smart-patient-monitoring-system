
import { useState } from 'react';
import axios from 'axios';
import { Link, useLocation, useNavigate } from 'react-router-dom';

function Login() {
  const navigate = useNavigate();
  const location = useLocation();

  // Check whether the user arrived after successful registration.
  const registrationSuccess = location.state?.registrationSuccess;

  // Login form state
  const [email, setEmail] = useState('');
  const [password, setPassword] = useState('');
  const [error, setError] = useState('');
  const [loading, setLoading] = useState(false);

  const handleLogin = async (event) => {
    event.preventDefault();

    setError('');
    setLoading(true);

    try {
      const response = await axios.post(
        'http://localhost:5000/api/v1/auth/login',
        {
          email,
          password
        }
      );

      const { token, user } = response.data;

      // Validate authentication response.
      if (!token) {
        throw new Error('No authentication token returned');
      }

      if (!['patient', 'caregiver'].includes(user?.role)) {
        throw new Error('Invalid user role returned');
      }

      // Store the JWT for authenticated API requests.
      sessionStorage.setItem('accessToken', token);

      // Store the user's role for frontend routing.
      // Backend authorization still validates the JWT.
      sessionStorage.setItem('userRole', user.role);

      console.log('Login successful:', user.role);

      // Redirect based on the authenticated user's role.
      if (user.role === 'caregiver') {
        navigate('/caregiver/dashboard', { replace: true });
      } else {
        navigate('/dashboard', { replace: true });
      }

    } catch (err) {
      setError(
        err.response?.data?.message ||
        err.message ||
        'Login failed'
      );
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="login-container">
      <form className="login-form" onSubmit={handleLogin}>
        <h1>SPMS</h1>
        <p>Smart Patient Monitoring System</p>

        {/* Display confirmation after successful signup */}
        {registrationSuccess && (
          <p className="signup-success" role="status">
            Account created successfully! You can now log in.
          </p>
        )}

        {/* Email input */}
        <label htmlFor="email">Email</label>
        <input
          id="email"
          type="email"
          value={email}
          onChange={(event) => setEmail(event.target.value)}
          required
        />

        {/* Password input */}
        <label htmlFor="password">Password</label>
        <input
          id="password"
          type="password"
          value={password}
          onChange={(event) => setPassword(event.target.value)}
          required
        />

        {/* Authentication errors */}
        {error && (
          <p className="login-error" role="alert">
            {error}
          </p>
        )}

        {/* Login button */}
        <button type="submit" disabled={loading}>
          {loading ? 'Signing in...' : 'Log In'}
        </button>

        {/* Link to account registration */}
        <p>
          Don't have an account?{' '}
          <Link to="/signup">Sign Up</Link>
        </p>
      </form>
    </div>
  );
}

export default Login;
