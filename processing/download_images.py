
import argparse as ap
from pathlib import Path
import io
from PIL import Image
import psycopg2 as pg
import toml



if __name__ == '__main__':
    parser = ap.ArgumentParser()
    parser.add_argument('-o', '--output-dir', type=str)
    args = parser.parse_args()

    with open('../server.toml', 'r') as f:
        config = toml.loads(f.read())

    def db_string(db_config):
        return (f'dbname={db_config["database"]} user={db_config["user"]} '
                f'password={db_config["password"]} host={db_config["host"]}')

    with pg.connect(db_string(config['db'])) as conn:
        with conn.cursor() as cur:
            if args.output_dir is None:
                output_dir = Path('.')
            else:
                output_dir = Path(args.output_dir)
                if not output_dir.exists():
                    output_dir.mkdir()
            cur.execute('SELECT * FROM images')
            for result in cur:
                out_filename = output_dir / f'{result[0]}.png'
                with open(out_filename, 'wb') as f:
                    f.write(result[1])
                    print(f'Wrote {out_filename}')
