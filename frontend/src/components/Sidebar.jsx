
import { NavLink, useNavigate } from 'react-router-dom';

function Sidebar() {

  // React Router function for navigating between pages.
  const navigate = useNavigate();

  // Read the user's role stored during login.
  const userRole = sessionStorage.getItem('userRole');

  // Determine which dashboard the user should access.
  const dashboardPath =
    userRole === 'caregiver'
      ? '/caregiver/dashboard'
      : '/dashboard';

  // Handle user logout.
  const handleLogout = () => {

    // Remove the JWT stored when the user logged in.
    sessionStorage.removeItem('accessToken');

    // Remove the user's role so the next login starts fresh.
    sessionStorage.removeItem('userRole');

    // Redirect the user to the Login page.
    // replace: true prevents the dashboard from remaining
    // as the most recent entry in browser history.
    navigate('/login', { replace: true });
  };

  return (
    <aside className="sidebar">

      {/* Application branding */}
      <div className="sidebar-brand">
        <span className="brand-icon">♥</span>
        <div>
          <h2>SPMS</h2>
          <p>Patient Monitoring</p>
        </div>
      </div>

      {/* Navigation links */}
      <nav className="sidebar-nav">

        <NavLink to={dashboardPath}>
          Dashboard
        </NavLink>

        <NavLink to="/alerts">
          Alerts
        </NavLink>

        <NavLink to="/devices">
          Devices
        </NavLink>

        <NavLink to="/profile">
          Profile
        </NavLink>

      </nav>

      {/* Logout button */}
      <div className="sidebar-footer">
        <button
          type="button"
          onClick={handleLogout}
        >
          Log Out
        </button>
      </div>

    </aside>
  );
}

export default Sidebar;
