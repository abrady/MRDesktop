module.exports = {
  root: true,
  parser: '@typescript-eslint/parser',
  plugins: ['@typescript-eslint','import'],
  extends: ['airbnb-typescript/base', 'prettier'],
  parserOptions: {
    project: "./tsconfig.eslint.json",
  },
  rules: {
    'no-console': 'off',
    'import/extensions': 'off',
    'import/no-extraneous-dependencies': 'off',
    '@typescript-eslint/no-redeclare': 'off',
  },
};
