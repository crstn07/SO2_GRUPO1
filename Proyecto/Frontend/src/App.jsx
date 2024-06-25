import React, { useState, useEffect } from 'react';
import { Dash } from "./Pages/dashboard";
import { Solicitud } from "./Pages/Solicitud";
import "./Diseño/App.css";

function App() {
  const [mostrarDash, setmostrarDash] = useState(false);
  const [mostrarSol, setmostrarSol] = useState(false);

  const [global, setglobal] = useState({});
  const [ListS, SetListS] = useState([]);

  

  // Función para actualizar el dashboard cada 3 segundos
  useEffect(() => {
      const obtenerDatosDashboard = () => {
        fetch("http://localhost:3000/dashboard",{
          method: "GET",
          headers: {
            "Content-Type": "application/json",
          },
        })
        .then((response) => response.json())
        .then((data) => {
    
          setglobal(data);
        })
        .catch((error) => {
          console.error("Error:", error);
        });
      };
      
    obtenerDatosDashboard();
    setmostrarDash(!mostrarDash);
    const intervalId = setInterval(obtenerDatosDashboard, 3000);

    return () => clearInterval(intervalId);
  }, []); // 

  const Dashboard = ()=> {
    setmostrarSol(false);
    setmostrarDash(!mostrarDash);
  };

  const Solicitudes = ()=> {
    setmostrarDash(false);
    setmostrarSol(!mostrarSol);

    fetch("http://localhost:3000/solicitud",{
      method: "GET",
      headers: {
        "Content-Type": "application/json",
      },
    })
    .then((response) => response.json())
    .then((data) => {
      SetListS(data);
    })
    .catch((error) => {
      console.error("Error:", error);
    });
  };

  return (
    <div>
      <h1>SO2 Proyecto Grupo1</h1>
      <div className="container">
        <div className="button-container">
          <button onClick={Dashboard}>Dashboard</button>
          <button onClick={Solicitudes}>Solicitudes</button>
        </div>
        {mostrarDash && (
            <div className="content">
              <Dash ListParms={global} />
            </div>
         )}
         {mostrarSol && (
            <div className="content">
              <Solicitud TableList={ListS}/>
            </div>
         )}
      </div>
    </div>
  );
}

export default App;
