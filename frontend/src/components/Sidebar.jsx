import { NavLink } from 'react-router-dom';

function Sidebar() {
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
        <button type="button">Log Out</button>
      </div>
    </aside>
  );
}

export default Sidebar;