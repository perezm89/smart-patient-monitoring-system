
import { NavLink, useNavigate } from 'react-router-dom';

function Sidebar() {
  // React Router function for navigating between pages.
  const navigate = useNavigate();

  // Handle user logout.
  const handleLogout = () => {
    // Remove the JWT stored when the user logged in.
    sessionStorage.removeItem('accessToken');

    // Redirect the user to the Login page.
    // replace: true prevents the dashboard from remaining
    // as the most recent entry in browser history.
    navigate('/login', { replace: true });
  };

  return (
    <aside className="sidebar">
      <div className="sidebar-brand">
        <span className="brand-icon">♥</span>
        <div>
          <h2>SPMS</h2>
          <p>Patient Monitoring</p>
        </div>
      </div>

      <nav className="sidebar-nav">
        <NavLink to="/dashboard">Dashboard</NavLink>
        <NavLink to="/vitals">Vitals</NavLink>
        <NavLink to="/alerts">Alerts</NavLink>
        <NavLink to="/devices">Devices</NavLink>
        <NavLink to="/profile">Profile</NavLink>
      </nav>

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
