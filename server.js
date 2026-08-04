const express = require('express');
const cors = require('cors');
const { exec } = require('child_process');
const path = require('path');

const app = express();
const PORT = 3535;

app.use(cors());
app.use(express.json());

app.post('/control', (req, res) => {
    const { action } = req.body;
    
    if (!['toggle', 'next', 'previous'].includes(action)) {
        return res.status(400).json({ error: 'Ação inválida' });
    }

    const scriptPath = path.join(__dirname, 'spotify-control.ps1');
    const command = `powershell -NoProfile -ExecutionPolicy Bypass -OutputFormat Text -File "${scriptPath}" -Action ${action}`;

    exec(command, { encoding: 'utf8' }, (error, stdout, stderr) => {
        if (error) {
            console.error(`Erro ao executar o script: ${error.message}`);
            return res.status(500).json({ error: error.message });
        }
        res.json({ success: true, message: stdout.trim() });
    });
});

app.listen(PORT, () => {
    console.log(`Spotify Bridge rodando na porta ${PORT}`);
});