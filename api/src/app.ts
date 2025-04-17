import express from 'express';
import 'fs'

const app = express();
const port = 3000;

app.use(express.json());

app.post('/', (req, res) => {
  // Logique de traitement de la requête POST
  console.log(req.body.key); // Affiche le corps de la requête dans la console
  res.status(200).send();
  /*   // Vérification si la clé API est présente dans le corps de la requête
   */
 /*  if (req.body && req.body.key) {
    // Logique de traitement de la key
    console.log(`Clé API reçue : ${req.body.key}`);
    //Check if the key is 128 bits (16 bytes)
    if (req.body.key.length !== 16) {
      return res.status(400).send('La clé API doit être de 128 bits (16 octets).');
    }   
    res.status(200).send(`Hello, ${req.body.name}!`);
    
  } */
});

app.listen(port, () => {
  console.log(`Serveur en écoute sur http://localhost:${port}`);
});
